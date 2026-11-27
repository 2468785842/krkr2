#include "CiallangRuntime.h"
#include "tjsError.h"
#include "tjsException.h"
#include "tjsNative.h"
#include "tjsScriptBlock.h"

#include <cial/Engine.hpp>
#include <types/Array.hpp>
#include <types/Class.hpp>
#include <types/Dictionary.hpp>
#include <types/Property.hpp>
#include <common/UTF16.hpp>
#include <cstdio>
#include <map>
#include <memory>
#include <optional>
#include <set>

namespace TJS {
    namespace {
        using cial::Value;
        using Handle = cial::VM::Handle<Value>;
        class ScriptObject;
        class HostObject;

        std::string utf8(const ttstr &text) {
            return cial::Common::fromUtf16(
                std::u16string_view(text.c_str(), text.GetLen()));
        }

        ttstr utf16(const std::string &text) {
            const auto wide = cial::Common::toUtf16(text);
            return ttstr(wide.c_str(), static_cast<tjs_int>(wide.size()));
        }

        // Own the SDK components explicitly so native inheritance adapters can
        // access the active receiver without exposing VM types to C++17
        // callers.
        struct VMOwner {
            cial::Runtime runtime_{ 65536 };
            cial::Context context_{ runtime_ };
            cial::vm::VMState state_{ context_ };
            cial::VM vm_{ &state_ };
            VMOwner() {
                cial::stdlib::registerStandardLibrary(context_);
                cial::stdlib::registerScriptsLib(context_, &state_);
            }
            cial::Runtime &runtime() { return runtime_; }
            cial::Context &context() { return context_; }
            cial::VM &vm() { return vm_; }
            Handle execute(const std::string &text,
                           cial::Common::Result &diagnostics,
                           const std::string &name, bool expression,
                           const Value &receiver) {
                cial::Common::SourceFile source;
                source.setSourceName(name);
                source.load(diagnostics, text);
                return vm_.execute(source, diagnostics, expression, receiver);
            }
        };

        struct State : std::enable_shared_from_this<State> {
            std::unique_ptr<VMOwner> engine = std::make_unique<VMOwner>();
            std::map<cial::Object *, ScriptObject *> scripts;
            std::map<std::pair<iTJSDispatch2 *, iTJSDispatch2 *>, HostObject *>
                hosts;
            bool closing = false;
            tTJS *owner = nullptr;

            cial::Runtime &rt() { return engine->runtime(); }
            cial::Atom atom(const tjs_char *name) {
                return rt().atomTable.intern(cial::String(utf8(ttstr(name))));
            }
            ttstr name(cial::Atom key) {
                return utf16(rt().atomTable.get(key)->str->toStdStr());
            }
            Value fromHost(const tTJSVariant &value);
            tTJSVariant toHost(const Value &value);
            void shutdown();
            [[noreturn]] void raise(const Value &value,
                                    const ttstr &fallbackName,
                                    const std::string &fallbackSource);
            Value initializeNative(const tTJSVariant &klass,
                                   cial::DataObject *instance, size_t count,
                                   const Value *args);
        };

        void check(tjs_error error, const char *context = nullptr) {
            if(TJS_FAILED(error)) {
                std::string message =
                    "TJS dispatch failed: " + std::to_string(error);
                if(context) {
                    message += " in ";
                    message += context;
                }
                throw std::runtime_error(message);
            }
        }

        // A GC-owned wrapper retains the complete TJS closure, including
        // ObjThis. The cache is weak: only live wrappers remain in it.
        class HostObject final : public cial::ClassObject {
        public:
            State &state;
            tTJSVariant closure;
            // 换绑（incontextof）产生的副本不参与 state.hosts 缓存：那套缓存
            // 以 (Object, ObjThis) 为键持有唯一包装，副本若也 erase 会把原
            // 包装从缓存里踢掉。
            bool cached{ true };

            HostObject(State &owner, const tTJSVariant &value,
                       cial::ClassMeta *metadata, bool cacheEntry = true) :
                cial::ClassObject(metadata, &owner.rt()), state(owner),
                closure(value), cached(cacheEntry) {}
            cial::TypeId typeId() const noexcept override {
                return cial::TypeId::Object;
            }
            ~HostObject() noexcept override {
                if(!cached)
                    return;
                state.hosts.erase({ closure.AsObjectNoAddRef(),
                                    closure.AsObjectThisNoAddRef() });
            }
            iTJSDispatch2 *object() const { return closure.AsObjectNoAddRef(); }
            iTJSDispatch2 *receiver() const {
                auto *self = closure.AsObjectThisNoAddRef();
                return self ? self : object();
            }
            // Native members read through a class object (`super.fullScreen`,
            // `super.enabled`) target the *instance*: native property getters do
            // NativeInstanceSupport(GETINSTANCE) on objthis and return
            // TJS_E_NATIVECLASSCRASH for a bare class object. When this wrapper
            // represents a class and the VM is executing a script instance,
            // dispatch on that instance's host proxy instead.
            // Owns a temporary receiver proxy for the duration of one dispatch.
            // Kept as a nested type so getProp/setProp/hasProp/call share one
            // ownership rule; an early or duplicated Release here is a
            // use-after-free because the same pointer may be stored in a
            // returned closure's ObjThis.
            struct ReceiverGuard {
                iTJSDispatch2 *value;
                bool owned;
                ReceiverGuard(iTJSDispatch2 *target, bool isOwned) :
                    value(target), owned(isOwned) {}
                ~ReceiverGuard() {
                    if(owned && value)
                        value->Release();
                }
            };
            iTJSDispatch2 *memberReceiver(bool &owned) const {
                owned = false;
                if(auto *self = closure.AsObjectThisNoAddRef())
                    return self;
                if(dynamic_cast<tTJSNativeClass *>(closure.AsObjectNoAddRef())) {
                    auto *frame = state.engine->state_.curFrame();
                    // Only a script *instance* can carry a native instance;
                    // class-level reads keep the native class object. The
                    // returned proxy performs its own liveness check on every
                    // dispatch, so no extra type test is needed here.
                    if(frame && frame->thisData && frame->thisData->nativeBase()) {
                        auto value = const_cast<State &>(state).toHost(
                            Value(frame->thisData));
                        if(auto *proxy = value.AsObjectNoAddRef()) {
                            proxy->AddRef();
                            owned = true;
                            return proxy;
                        }
                    }
                }
                return object();
            }
            Value getProp(cial::Atom key) override {
                if(meta && key == meta->className) {
                    auto *frame = state.engine->state_.curFrame();
                    auto *instance = frame ? frame->thisData : nullptr;
                    if(!instance)
                        throw std::runtime_error("Native base constructor "
                                                 "requires a script instance");
                    auto constructor = state.rt().create<cial::NativeFunction>(
                        [owner = &state, klass = closure,
                         instance](cial::VM *, const Value &, size_t count,
                                   Value *args) {
                            return owner->initializeNative(klass, instance,
                                                           count, args);
                        });
                    constructor->setThisObj(Value(instance));
                    return Value(constructor.get());
                }
                const auto member = state.name(key);
                tTJSVariant result;
                bool owned = false;
                auto *target = memberReceiver(owned);
                // Hold the temporary proxy for the whole body: the closure
                // returned below may be rebound to it, and a Release before
                // that binding (or a second Release after it) is a
                // use-after-free.
                const ReceiverGuard receiverGuard{ target, owned };
                const auto status = object()->PropGet(
                    0, member.c_str(), nullptr, &result, target);
                // TJS2 属性读取未命中返回 void；KAG 用
                // typeof global.Layer.copyRaster == "undefined"
                // 探测原生类成员， 这里抛异常会中断整段脚本。
                if(status == TJS_E_MEMBERNOTFOUND)
                    return {};
                if(TJS_FAILED(status)) {
                    std::string context = "HostObject::getProp ";
                    context += utf8(member);
                    context += " object=" +
                        std::to_string(
                                   reinterpret_cast<std::uintptr_t>(object()));
                    context += " receiver=" +
                        std::to_string(reinterpret_cast<std::uintptr_t>(
                            receiver()));
                    check(status, context.c_str());
                }
                // Rebind only when this read substituted the receiving instance
                // for a native class object (`super.fullScreen`,
                // `super.setZoom`). Plain data reads — `children[i]` returning a
                // MenuItem, dictionary lookups — legitimately produce closures
                // without ObjThis; binding those to the container would make the
                // next member access resolve a native instance on the wrong
                // object (`item.index` then fails converting an object to
                // Integer).
                if(owned && result.Type() == tvtObject &&
                   !result.AsObjectThisNoAddRef())
                    result.ChangeClosureObjThis(target);
                return state.fromHost(result);
            }
            void setProp(cial::Atom key, const Value &value) override {
                const auto member = state.name(key);
                auto converted = state.toHost(value);
                bool owned = false;
                auto *target = memberReceiver(owned);
                const ReceiverGuard receiverGuard{ target, owned };
                const auto status =
                    object()->PropSet(TJS_MEMBERENSURE, member.c_str(), nullptr,
                                      &converted, target);
                if(TJS_FAILED(status)) {
                    std::string context = "HostObject::setProp ";
                    context += utf8(member);
                    context += " type=";
                    context += std::to_string(converted.Type());
                    context += " object=" +
                        std::to_string(
                                   reinterpret_cast<std::uintptr_t>(object()));
                    context += " receiver=" +
                        std::to_string(reinterpret_cast<std::uintptr_t>(
                            receiver()));
                    check(status, context.c_str());
                }
            }
            bool hasProp(cial::Atom key) const override {
                const auto member = state.name(key);
                tTJSVariant ignored;
                bool owned = false;
                auto *target = memberReceiver(owned);
                const ReceiverGuard receiverGuard{ target, owned };
                const auto result =
                    object()->PropGet(TJS_IGNOREPROP, member.c_str(), nullptr,
                                      &ignored, target);
                if(result == TJS_E_MEMBERNOTFOUND)
                    return false;
                check(result);
                return true;
            }
            bool delProp(cial::Atom key) override {
                const auto member = state.name(key);
                const auto result = object()->DeleteMember(0, member.c_str(),
                                                           nullptr, receiver());
                if(result == TJS_E_MEMBERNOTFOUND)
                    return false;
                check(result);
                return true;
            }
            bool instanceOf(cial::Atom key) override {
                const auto type = state.name(key);
                return object()->IsInstanceOf(0, nullptr, nullptr, type.c_str(),
                                              receiver()) == TJS_S_TRUE;
            }
            void call(cial::vm::VMState &vm, cial::u32 result,
                      size_t count) override {
                std::vector<tTJSVariant> args;
                std::vector<tTJSVariant *> pointers;
                auto *source = vm.curFrame()->getArgs(count);
                args.reserve(count);
                pointers.reserve(count);
                for(size_t i = 0; i < count; ++i)
                    args.push_back(state.toHost(source[i]));
                for(auto &arg : args)
                    pointers.push_back(&arg);
                // A script constructor may invoke a native base by bare name
                // (`WaveSoundBuffer(owner)` in BGM.tjs) instead of
                // `super.WaveSoundBuffer()`. TJS2 runs that as FuncCall with the
                // current instance as objthis, registering the native instance
                // and its members on that object. Creating a new object instead
                // would leave the derived instance without a native base and
                // fail the derived-constructor base check.
                // Ciallang lowers `new X(...)` and `X(...)` to the same call
                // opcode, so the VM cannot tell them apart. Narrow to the one
                // case the language needs: the callee is a direct native base
                // of the constructor's own class. `new Layer(win, this)` must
                // not take this path — Layer is only an indirect ancestor
                // there, and joining a layer to itself throws.
                auto *nativeClass = dynamic_cast<tTJSNativeClass *>(object());
                if(nativeClass && vm.curFrame()->thisData &&
                   vm.curFrame()->funcMeta) {
                    auto *instance = vm.curFrame()->thisData;
                    const auto nativeName =
                        state.atom(nativeClass->getClassName().c_str());
                    const auto *owner =
                        vm.curFrame()->funcMeta->ownerClass;
                    const bool directBase =
                        owner && std::ranges::find(owner->extends, nativeName) !=
                            owner->extends.end();
                    if(directBase) {
                        vm.regRef(static_cast<cial::u16>(result)) =
                            state.initializeNative(closure, instance, count,
                                                   source);
                        return;
                    }
                }
                tTJSVariant returned;
                tjs_error status;
                bool ownedReceiver = false;
                auto *callReceiver = memberReceiver(ownedReceiver);
                const ReceiverGuard receiverGuard{ callReceiver, ownedReceiver };
                if(object()->IsInstanceOf(0, nullptr, nullptr, TJS_W("Class"),
                                          callReceiver) == TJS_S_TRUE) {
                    iTJSDispatch2 *instance = nullptr;
                    status = object()->CreateNew(0, nullptr, nullptr, &instance,
                                                 static_cast<tjs_int>(count),
                                                 pointers.data(), callReceiver);
                    if(TJS_SUCCEEDED(status) && instance) {
                        returned = tTJSVariant(instance, instance);
                        instance->Release();
                    }
                } else {
                    status = object()->FuncCall(0, nullptr, nullptr, &returned,
                                                static_cast<tjs_int>(count),
                                                pointers.data(), callReceiver);
                }
                check(status, "HostObject::call");
                vm.regRef(static_cast<cial::u16>(result)) =
                    state.fromHost(returned);
            }
        };

        // The host sees a normal dispatch object; a Handle keeps its script
        // value alive until the last host reference is released or the engine
        // shuts down.
        class ScriptObject final : public tTJSCustomObject {
        public:
            std::shared_ptr<State> state;
            std::optional<Handle> root;
            cial::Object *identity;
            bool installingNative = false;

            ScriptObject(std::shared_ptr<State> owner, const Value &value) :
                state(std::move(owner)),
                root(std::in_place, &state->rt(), value),
                identity(value.requireObject()) {
                // This is a host reference to a GC-owned value, not a second
                // owner of its script finalizer. Releasing a temporary proxy
                // must not finalize a value which the VM still holds.
                tTJSVariant noFinalizer(TJS_W(""));
                tTJSCustomObject::ClassInstanceInfo(TJS_CII_SET_FINALIZE, 0,
                                                    &noFinalizer);
                state->scripts.emplace(identity, this);
            }
            ~ScriptObject() override { state->scripts.erase(identity); }

            bool active() const { return root.has_value() && !state->closing; }
            const Value &value() const { return *root->operator->(); }
            std::optional<Value> findMember(const tjs_char *name) {
                const auto key = state->atom(name);
                // Match VM member reads: native members precede object slots.
                const auto native = state->engine->context().findNativeMember(
                    identity->nativeTypeKey(), key);
                if(native.getter)
                    return native.getter(value());
                if(native.accessor.getter) {
                    auto getter = state->rt().create<cial::NativeFunction>(
                        *native.accessor.getter);
                    getter->setThisObj(value());
                    return state->engine->vm().callValue(Value(getter.get()), 0,
                                                         nullptr);
                }
                if(native.method) {
                    auto method = state->rt().create<cial::NativeFunction>(
                        *native.method);
                    method->setThisObj(value());
                    return Value(method.get());
                }
                if(identity->hasProp(key))
                    return identity->getProp(key);
                if(identity->typeId() == cial::TypeId::Class) {
                    auto *klass = static_cast<cial::ClassObject *>(identity);
                    if(auto *meta = cial::findClassMethod(state->engine->state_,
                                                          klass->meta, key)) {
                        return Value(
                            state->rt().create<cial::Function>(meta).get());
                    }
                }
                return std::nullopt;
            }
            Value member(const tjs_char *name) {
                return name ? findMember(name).value_or(Value{}) : value();
            }
            tjs_error PropGet(tjs_uint32 flags, const tjs_char *name,
                              tjs_uint32 *, tTJSVariant *result,
                              iTJSDispatch2 *) override {
                if(!active())
                    return TJS_E_INVALIDOBJECT;
                if(!name)
                    return TJS_E_INVALIDTYPE;
                auto found = findMember(name);
                if(!found)
                    return TJS_E_MEMBERNOTFOUND;
                auto current = *found;
                if(!(flags & TJS_IGNOREPROP) && current.isObject() &&
                   !current.isNull() &&
                   current.requireObject()->typeId() ==
                       cial::TypeId::Property) {
                    auto *property =
                        static_cast<cial::Property *>(current.requireObject());
                    if(!property->propMeta->getFunc)
                        return TJS_E_ACCESSDENYED;
                    auto fn = state->rt().create<cial::Function>(
                        property->propMeta->getFunc);
                    fn->thisObj = property->thisObj;
                    current = state->engine->vm().callValue(Value(fn.get()), 0,
                                                            nullptr);
                }
                if(result)
                    *result = state->toHost(current);
                return TJS_S_OK;
            }
            tjs_error PropSet(tjs_uint32 flags, const tjs_char *name,
                              tjs_uint32 *, const tTJSVariant *input,
                              iTJSDispatch2 *) override {
                if(installingNative)
                    return tTJSCustomObject::PropSet(flags, name, nullptr,
                                                     input, this);
                if(!active())
                    return TJS_E_INVALIDOBJECT;
                // TJS probes an existing object as a property setter before
                // replacing its slot. Ordinary script objects have no default
                // setter; INVALIDTYPE lets TJS continue with substitution.
                if(!name)
                    return TJS_E_INVALIDTYPE;
                if(!input)
                    return TJS_E_INVALIDPARAM;
                const auto key = state->atom(name);
                if(!(flags & TJS_MEMBERENSURE) && !identity->hasProp(key))
                    return TJS_E_MEMBERNOTFOUND;
                identity->setProp(key, state->fromHost(*input));
                return TJS_S_OK;
            }
            tjs_error PropSetByVS(tjs_uint32 flags, tTJSVariantString *name,
                                  const tTJSVariant *input,
                                  iTJSDispatch2 *self) override {
                return PropSet(flags, name->operator const tjs_char *(),
                               nullptr, input, self);
            }
            tjs_error FuncCall(tjs_uint32, const tjs_char *name, tjs_uint32 *,
                               tTJSVariant *result, tjs_int count,
                               tTJSVariant **args,
                               iTJSDispatch2 *self) override {
                if(!active())
                    return TJS_E_INVALIDOBJECT;
                auto found =
                    name ? findMember(name) : std::optional<Value>(value());
                if(!found)
                    return TJS_E_MEMBERNOTFOUND;
                Handle callable(&state->rt(), *found);
                // Inherited native methods retain their original TJS closure.
                // VM::callValue accepts script/native SDK functions, whereas a
                // HostObject is a dispatch adapter with its own call protocol.
                if(callable->isObject() && !callable->isNull()) {
                    if(auto *host = dynamic_cast<HostObject *>(
                           callable->requireObject())) {
                        return host->closure.AsObjectClosureNoAddRef().FuncCall(
                            0, nullptr, nullptr, result, count, args,
                            self ? self : host->receiver());
                    }
                }
                std::vector<Handle> roots;
                std::vector<Value> values;
                for(tjs_int i = 0; i < count; ++i) {
                    roots.emplace_back(&state->rt(), state->fromHost(*args[i]));
                    values.push_back(*roots.back());
                }
                const auto receiver =
                    self ? state->fromHost(tTJSVariant(self, self)) : value();
                // A direct dispatch call can supply an instance for a method
                // obtained from its class (the TJS native-method contract).
                // Ordinary exported closures pass this proxy as self and keep
                // the receiver captured by the VM member read.
                if(self && self != this && callable->isObject() &&
                   !callable->isNull()) {
                    auto *object = callable->requireObject();
                    if(object->typeId() == cial::TypeId::NativeFunction) {
                        auto method = state->rt().create<cial::NativeFunction>(
                            *static_cast<cial::NativeFunction *>(object));
                        method->setThisObj(receiver);
                        callable = Handle(&state->rt(), Value(method.get()));
                    } else if(object->typeId() == cial::TypeId::Function) {
                        // Explicit receiver wins over the caller's this for
                        // script methods too. Clone instead of assigning
                        // thisObj: the stored Function is shared by every holder
                        // of that member, so mutating it would bind every other
                        // instances.
                        auto method = state->rt().create<cial::Function>(
                            static_cast<cial::Function *>(object)->meta);
                        method->thisObj = receiver.requireObject();
                        callable = Handle(&state->rt(), Value(method.get()));
                    }
                }
                Handle returned(
                    &state->rt(),
                    state->engine->vm().callValue(*callable, values.size(),
                                                  values.data(), receiver));
                if(result)
                    *result = state->toHost(*returned);
                return TJS_S_OK;
            }
            tjs_error DeleteMember(tjs_uint32, const tjs_char *name,
                                   tjs_uint32 *, iTJSDispatch2 *) override {
                if(!active())
                    return TJS_E_INVALIDOBJECT;
                if(!name)
                    return TJS_E_INVALIDPARAM;
                return identity->delProp(state->atom(name))
                    ? TJS_S_OK
                    : TJS_E_MEMBERNOTFOUND;
            }
            tjs_error IsInstanceOf(tjs_uint32, const tjs_char *memberName,
                                   tjs_uint32 *, const tjs_char *className,
                                   iTJSDispatch2 *) override {
                if(!active())
                    return TJS_E_INVALIDOBJECT;
                const auto object = member(memberName);
                return object.isObject() && !object.isNull() &&
                        object.requireObject()->instanceOf(
                            state->atom(className))
                    ? TJS_S_TRUE
                    : TJS_S_FALSE;
            }
        };

        class NativeFacet final : public cial::Object {
            State &state;
            ScriptObject *proxy;

        public:
            NativeFacet(State &owner, ScriptObject *object) :
                state(owner), proxy(object) {
                proxy->AddRef();
            }
            ~NativeFacet() noexcept override { proxy->Release(); }
            Value getProp(cial::Atom key) override {
                const auto name = state.name(key);
                tTJSVariant result;
                check(proxy->tTJSCustomObject::PropGet(0, name.c_str(), nullptr,
                                                       &result, proxy));
                return state.fromHost(result);
            }
            void setProp(cial::Atom key, const Value &value) override {
                const auto name = state.name(key);
                auto input = state.toHost(value);
                check(proxy->tTJSCustomObject::PropSet(0, name.c_str(), nullptr,
                                                       &input, proxy));
            }
            bool hasProp(cial::Atom key) const override {
                const auto name = state.name(key);
                tTJSVariant ignored;
                const auto status = proxy->tTJSCustomObject::PropGet(
                    TJS_IGNOREPROP, name.c_str(), nullptr, &ignored, proxy);
                if(status == TJS_E_MEMBERNOTFOUND)
                    return false;
                check(status);
                return true;
            }
            bool delProp(cial::Atom key) override {
                const auto name = state.name(key);
                const auto status = proxy->tTJSCustomObject::DeleteMember(
                    0, name.c_str(), nullptr, proxy);
                if(status == TJS_E_MEMBERNOTFOUND)
                    return false;
                check(status);
                return true;
            }
            bool instanceOf(cial::Atom key) override {
                const auto name = state.name(key);
                return proxy->tTJSCustomObject::IsInstanceOf(
                           0, nullptr, nullptr, name.c_str(), proxy) ==
                    TJS_S_TRUE;
            }
            void call(cial::vm::VMState &, cial::u32, size_t) override {
                throw std::runtime_error(
                    "A native instance member facet is not callable");
            }
        };

        Value State::initializeNative(const tTJSVariant &klass,
                                      cial::DataObject *instance, size_t count,
                                      const Value *args) {
            auto *nativeClass =
                dynamic_cast<tTJSNativeClass *>(klass.AsObjectNoAddRef());
            if(!nativeClass)
                throw std::runtime_error("Invalid native superclass");
            auto reference = toHost(Value(instance));
            auto *proxy =
                dynamic_cast<ScriptObject *>(reference.AsObjectNoAddRef());
            if(!proxy)
                throw std::runtime_error("Invalid native instance dispatch");
            struct Installing {
                ScriptObject &proxy;
                explicit Installing(ScriptObject &p) : proxy(p) {
                    proxy.installingNative = true;
                }
                ~Installing() { proxy.installingNative = false; }
            } installing(*proxy);
            check(nativeClass->FuncCall(0, nullptr, nullptr, nullptr, 0,
                                        nullptr, proxy));
            auto facet = rt().create<NativeFacet>(*this, proxy);
            instance->setNativeBase(facet.get());
            auto base = fromHost(klass);
            instance->setSuperClass(
                atom(nativeClass->getClassName().c_str()),
                dynamic_cast<cial::ClassObject *>(base.requireObject()));
            std::vector<tTJSVariant> values;
            std::vector<tTJSVariant *> pointers;
            values.reserve(count);
            for(size_t i = 0; i < count; ++i)
                values.push_back(toHost(args[i]));
            for(auto &arg : values)
                pointers.push_back(&arg);
            check(nativeClass->FuncCall(
                0, nativeClass->getClassName().c_str(), nullptr, nullptr,
                static_cast<tjs_int>(count), pointers.data(), proxy));
            return Value(instance);
        }

        Value State::fromHost(const tTJSVariant &value) {
            switch(value.Type()) {
                case tvtVoid:
                    return {};
                case tvtInteger:
                    return Value(cial::Integer(value.AsInteger()));
                case tvtReal:
                    return Value(cial::Real(value.AsReal()));
                case tvtString:
                    return Value(cial::String(utf8(ttstr(value))));
                case tvtOctet: {
                    const auto *octet = value.AsOctetNoAddRef();
                    return Value(
                        cial::Octet(octet->GetData(), octet->GetLength()));
                }
                case tvtObject: {
                    auto *object = value.AsObjectNoAddRef();
                    if(!object)
                        return Value(static_cast<cial::Object *>(nullptr));
                    if(auto *script = dynamic_cast<ScriptObject *>(object)) {
                        if(script->state.get() != this || !script->active())
                            throw std::runtime_error(
                                "Script object belongs to an inactive runtime");
                        // A host-held closure may carry an ObjThis that differs
                        // from the object itself (`this.handler = with_target`).
                        // Handing back the bare script function would drop that
                        // receiver, so bind a copy: mutating the shared
                        // Function::thisObj would leak the receiver into every
                        // other instance holding the same closure.
                        const auto source = script->value();
                        auto *boundTo = value.AsObjectThisNoAddRef();
                        if(boundTo && boundTo != object &&
                           source.isObject() && !source.isNull() &&
                           source.requireObject()->typeId() ==
                               cial::TypeId::Function) {
                            Handle receiverRoot(
                                &rt(), fromHost(tTJSVariant(boundTo, boundTo)));
                            auto *original =
                                static_cast<cial::Function *>(
                                    source.requireObject());
                            auto bound =
                                rt().create<cial::Function>(original->meta);
                            bound->thisObj = receiverRoot->requireObject();
                            return Value(bound.get());
                        }
                        return source;
                    }
                    const auto key =
                        std::make_pair(object, value.AsObjectThisNoAddRef());
                    if(auto it = hosts.find(key); it != hosts.end())
                        return Value(it->second);
                    std::optional<cial::RootGuard<cial::ClassMeta>> metadata;
                    if(auto *klass = dynamic_cast<tTJSNativeClass *>(object))
                        metadata.emplace(rt().create<cial::ClassMeta>(
                            atom(klass->getClassName().c_str()), 0));
                    auto wrapped = rt().create<HostObject>(
                        *this, value, metadata ? metadata->get() : nullptr);
                    hosts.emplace(key, wrapped.get());
                    return Value(wrapped.get());
                }
            }
            throw std::runtime_error("Unsupported host value");
        }

        tTJSVariant State::toHost(const Value &value) {
            if(value.isVoid())
                return {};
            if(value.isInteger())
                return tTJSVariant(
                    static_cast<tjs_int64>(value.asInteger().unwrap()));
            if(value.isReal())
                return tTJSVariant(
                    static_cast<tjs_real>(value.asReal().unwrap().value()));
            if(value.isString())
                return tTJSVariant(
                    utf16(value.asString().unwrap()->toStdStr()));
            if(value.isOctet()) {
                const auto *octet = value.asOctet().unwrap();
                return tTJSVariant(octet->getData(), octet->getSize());
            }
            if(value.isNull())
                return tTJSVariant(static_cast<iTJSDispatch2 *>(nullptr));
            auto *object = value.requireObject();
            if(auto *host = dynamic_cast<HostObject *>(object))
                return host->closure;
            if(auto it = scripts.find(object); it != scripts.end())
                return tTJSVariant(it->second, it->second);
            auto *proxy = new ScriptObject(shared_from_this(), value);
            tTJSVariant returned(proxy, proxy);
            proxy->Release();
            return returned;
        }

        struct ReleaseBlock {
            void operator()(tTJSScriptBlock *block) const { block->Release(); }
        };

        [[noreturn]] void State::raise(const Value &value,
                                       const ttstr &fallbackName,
                                       const std::string &fallbackSource) {
            const auto fault = engine->state_.fault();
            auto source = fallbackSource;
            auto name = fallbackName.IsEmpty() ? ttstr(TJS_W("<script>"))
                                               : fallbackName;
            if(fault.valid && !fault.source.empty())
                name = utf16(fault.source);
            if(fault.snapshot) {
                const auto &bytes = fault.snapshot->buffer();
                source.assign(reinterpret_cast<const char *>(bytes.data()),
                              bytes.size());
            } else if(fault.valid && source.empty()) {
                source.assign(static_cast<size_t>(fault.line), '\n');
            }
            std::unique_ptr<tTJSScriptBlock, ReleaseBlock> block(
                tTJSScriptBlock::CreateDiagnostic(owner, utf16(source), name));
            const auto line =
                fault.valid ? static_cast<tjs_int>(fault.line) : 0;
            const auto position = block->LineToSrcPos(line);
            auto thrown = toHost(value);
            ttstr message(TJS_W("Ciallang script exception"));
            if(thrown.Type() != tvtObject) {
                message = ttstr(thrown);
            } else if(auto *object = thrown.AsObjectNoAddRef()) {
                tTJSVariant detail;
                if(TJS_SUCCEEDED(
                       object->PropGet(0, TJS_W("message"), nullptr, &detail,
                                       thrown.AsObjectThisNoAddRef())))
                    message = ttstr(detail);
            }
            throw eTJSScriptException(message, block.get(), position, thrown);
        }

        void State::shutdown() {
            if(closing)
                return;
            closing = true;
            std::vector<ScriptObject *> snapshot;
            for(auto [object, proxy] : scripts) {
                proxy->AddRef();
                snapshot.push_back(proxy);
            }
            for(auto *proxy : snapshot) {
                proxy->Clear();
                proxy->root.reset();
            }
            engine.reset();
            for(auto *proxy : snapshot)
                proxy->Release();
        }

        // Use the same streams, encoding modes and virtual paths as TJS2's
        // Array.load/save (tjsArray.cpp), including XP3 and file:// storages.
        // `(orig incontextof lay)(...)` on a bridged native method: the VM cannot
        // clone a HostObject itself, so it asks this hook. Returning a fresh
        // HostObject whose TJS closure carries the new ObjThis keeps the
        // original method object (and every other holder of it) untouched.
        void installHostRebindHook(State &state) {
            state.engine->runtime().hostRebindHook =
                [](cial::Object *callable, cial::Object *newThis)
                -> cial::Object * {
                auto *host = dynamic_cast<HostObject *>(callable);
                if(!host || !newThis)
                    return nullptr;
                auto *receiver = dynamic_cast<ScriptObject *>(newThis);
                if(!receiver || !receiver->active())
                    return nullptr;
                auto closure = host->closure;
                closure.ChangeClosureObjThis(receiver);
                return host->state.rt()
                    .create<HostObject>(host->state, closure, nullptr,
                                        /*cacheEntry=*/false)
                    .get();
            };
        }

        void installArrayTextStorage(State &state) {
            state.engine->context()
                .nativeType<cial::Array>()
                .method(
                    cial::String("load"),
                    [&state](cial::VM *, const Value &self, size_t count,
                             Value *args) {
                        if(count < 1)
                            throw std::runtime_error(
                                "Array.load expects a filename");
                        auto *array =
                            dynamic_cast<cial::Array *>(self.requireObject());
                        if(!array)
                            throw std::runtime_error(
                                "Array.load expects an array");
                        const ttstr name(state.toHost(args[0]));
                        const ttstr mode = count > 1 && !args[1].isVoid()
                            ? ttstr(state.toHost(args[1]))
                            : ttstr();
                        auto *stream = TJSCreateTextStreamForRead(name, mode);
                        try {
                            array->elements.clear();
                            ttstr content;
                            stream->Read(content, 0);
                            const auto *start = content.c_str();
                            const auto *cursor = start;
                            while(*cursor) {
                                if(*cursor == TJS_W('\r') ||
                                   *cursor == TJS_W('\n')) {
                                    array->elements.emplace_back(cial::String(
                                        utf8(ttstr(start,
                                                   static_cast<tjs_int>(
                                                       cursor - start)))));
                                    const auto first = *cursor++;
                                    if(first == TJS_W('\r') &&
                                       *cursor == TJS_W('\n'))
                                        ++cursor;
                                    start = cursor;
                                } else
                                    ++cursor;
                            }
                            if(cursor != start)
                                array->elements.emplace_back(cial::String(
                                    utf8(ttstr(start,
                                               static_cast<tjs_int>(cursor -
                                                                    start)))));
                        } catch(...) {
                            stream->Destruct();
                            throw;
                        }
                        stream->Destruct();
                        return self;
                    })
                .method(
                    cial::String("save"),
                    [&state](cial::VM *, const Value &self, size_t count,
                             Value *args) {
                        if(count < 1)
                            throw std::runtime_error(
                                "Array.save expects a filename");
                        auto *array =
                            dynamic_cast<cial::Array *>(self.requireObject());
                        if(!array)
                            throw std::runtime_error(
                                "Array.save expects an array");
                        const ttstr name(state.toHost(args[0]));
                        const ttstr mode = count > 1 && !args[1].isVoid()
                            ? ttstr(state.toHost(args[1]))
                            : ttstr();
                        auto *stream = TJSCreateTextStreamForWrite(name, mode);
                        try {
#ifdef TJS_TEXT_OUT_CRLF
                            const ttstr newline(TJS_W("\r\n"));
#else
                            const ttstr newline(TJS_W("\n"));
#endif
                            for(const auto &item : array->elements) {
                                if(item.isString() || item.isInteger() ||
                                   item.isReal())
                                    stream->Write(ttstr(state.toHost(item)));
                                stream->Write(newline);
                            }
                        } catch(...) {
                            stream->Destruct();
                            throw;
                        }
                        stream->Destruct();
                        return self;
                    });
        }

        class Runtime final : public CiallangRuntime {
            std::shared_ptr<State> state = std::make_shared<State>();
            ScriptObject *global;

        public:
            Runtime(tTJS *owner, iTJSDispatch2 *nativeGlobal) {
                state->owner = owner;
                installArrayTextStorage(*state);
                installHostRebindHook(*state);
                global = new ScriptObject(
                    state, Value(state->engine->context().global()));
                class Import final : public tTJSDispatch {
                public:
                    State &state;
                    explicit Import(State &owner) : state(owner) {}
                    tjs_error FuncCall(tjs_uint32, const tjs_char *,
                                       tjs_uint32 *, tTJSVariant *result,
                                       tjs_int count, tTJSVariant **args,
                                       iTJSDispatch2 *) override {
                        if(count != 3)
                            return TJS_E_BADPARAMCOUNT;
                        const auto name = utf8(ttstr(*args[0]));
                        if(name != "Array" && name != "Dictionary" &&
                           name != "Math" && name != "Date" && name != "RegExp")
                            state.engine->context().registerGlobal(
                                cial::String(name), state.fromHost(*args[2]));
                        if(result)
                            *result = tjs_int(1);
                        return TJS_S_OK;
                    }
                } importer(*state);
                tTJSVariantClosure callback(&importer, nullptr);
                check(nativeGlobal->EnumMembers(TJS_IGNOREPROP, &callback,
                                                nativeGlobal));
            }
            ~Runtime() override {
                Shutdown();
                global->Release();
            }
            tTJSCustomObject *Global() override {
                global->AddRef();
                return global;
            }
            void Shutdown() override { state->shutdown(); }
            void SetPPValue(const tjs_char *name, tjs_int32 value) override {
                state->engine->context().pp().setVar(
                    cial::String(utf8(ttstr(name))), value);
            }
            void Collect() override {
                state->rt().markSweep.collect(
                    [&] { state->rt().collectAllMark(); });
            }
            void Execute(const ttstr &source, tTJSVariant *result,
                         iTJSDispatch2 *context, const ttstr &name,
                         tjs_int offset, bool expression) override {
                try {
                    if(offset < 0)
                        throw std::runtime_error("Negative script line offset");
                    cial::Common::Result diagnostics;
                    auto code = std::string(static_cast<size_t>(offset), '\n') +
                        utf8(source);
                    const auto receiver = context
                        ? state->fromHost(tTJSVariant(context, context))
                        : Value{};
                    auto returned = state->engine->execute(
                        code, diagnostics,
                        name.IsEmpty() ? "<script>" : utf8(name), expression,
                        receiver);
                    if(diagnostics.isFailed()) {
                        std::string message;
                        for(const auto &entry : diagnostics.messages()) {
                            if(!entry.source().empty())
                                message += entry.source();
                            if(entry.hasLocation()) {
                                message +=
                                    ":" +
                                    std::to_string(
                                        entry.location().start().line + 1) +
                                    ":" +
                                    std::to_string(
                                        entry.location().start().column + 1);
                            }
                            message += ": " + entry.message() + "\n";
                        }
                        throw std::runtime_error(message);
                    }
                    if(result)
                        *result = state->toHost(*returned);
                } catch(const cial::vm::ScriptExceptionCarrier &error) {
#ifdef CIAL_RET_DIAGNOSTICS
                    // Temporary diagnostics: KAG's exception handler can mask
                    // the original failure with a second dispatch exception.
                    const auto failures = cial::RetDebug::dumpFailures();
                    if(!failures.empty())
                        std::fprintf(stderr, "[CIAL_RET_DIAGNOSTICS] %s\n%s",
                                     utf8(name).c_str(), failures.c_str());
#endif
                    state->raise(
                        error.value, name,
                        std::string(static_cast<size_t>(offset), '\n') +
                            utf8(source));
                } catch(const std::exception &error) {
#ifdef CIAL_RET_DIAGNOSTICS
                    const auto failures = cial::RetDebug::dumpFailures();
                    std::fprintf(stderr, "[CIAL_RET_DIAGNOSTICS] %s: %s\n%s",
                                 utf8(name).c_str(), error.what(),
                                 failures.c_str());
#endif
                    // 运行时异常（属性缺失、不可索引等）本身不带位置；VM 已在
                    // 抛出前记录 fault，这里补文件名与行号，否则宿主只显示一句
                    // 纯文本，无法定位脚本。
                    std::string message = "Ciallang: " + utf8(name);
                    const auto fault = state->engine->state_.fault();
                    if(fault.valid && fault.line > 0) {
                        message += ":" + std::to_string(fault.line);
                        if(!fault.source.empty()) {
                            message += " (" + fault.source + ")";
                        }
                    }
                    message += ": ";
                    message += error.what();
                    TJS_eTJSError(utf16(message));
                }
            }
        };
    } // namespace

    namespace {
        std::map<tTJS *, std::unique_ptr<CiallangRuntime>> runtimes;
    }
    CiallangRuntime *AttachCiallangRuntime(tTJS *engine,
                                           iTJSDispatch2 *global) {
        auto runtime = std::make_unique<Runtime>(engine, global);
        auto *result = runtime.get();
        runtimes.emplace(engine, std::move(runtime));
        return result;
    }
    CiallangRuntime *FindCiallangRuntime(tTJS *engine) {
        const auto it = runtimes.find(engine);
        return it == runtimes.end() ? nullptr : it->second.get();
    }
    void ReleaseCiallangRuntime(tTJS *engine) { runtimes.erase(engine); }
} // namespace TJS
