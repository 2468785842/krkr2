#include <catch2/catch_test_macros.hpp>
#include "tjs.h"
#include "tjsObject.h"
#include "tjsNative.h"
#include "tjsArray.h"
#include "tjsError.h"
#include "tjsScriptBlock.h"
#include <memory>

using namespace TJS;

namespace {
    struct ReleaseEngine {
        void operator()(tTJS *engine) const {
            engine->Shutdown();
            engine->Release();
        }
    };
    using Engine = std::unique_ptr<tTJS, ReleaseEngine>;

    class HostCounter final : public tTJSDispatch {
    public:
        int calls = 0;
        tjs_error FuncCall(tjs_uint32, const tjs_char *name, tjs_uint32 *,
                           tTJSVariant *result, tjs_int count,
                           tTJSVariant **args, iTJSDispatch2 *self) override {
            if(name)
                return TJS_E_MEMBERNOTFOUND;
            if(self != this)
                return TJS_E_NATIVECLASSCRASH;
            if(count != 1)
                return TJS_E_BADPARAMCOUNT;
            ++calls;
            if(result)
                *result = tjs_int(*args[0]) + 1;
            return TJS_S_OK;
        }
    };
    struct CounterInstance final : tTJSNativeInstance {
        tjs_int value = 41;
        static CounterInstance *get(iTJSDispatch2 *self) {
            iTJSNativeInstance *instance = nullptr;
            const auto id = TJSFindNativeClassID(TJS_W("BridgeCounter"));
            if(!self ||
               TJS_FAILED(self->NativeInstanceSupport(TJS_NIS_GETINSTANCE, id,
                                                      &instance)))
                return nullptr;
            return static_cast<CounterInstance *>(instance);
        }
    };

    class NativeCounter final : public tTJSNativeClass {
        iTJSNativeInstance *CreateNativeInstance() override {
            return new CounterInstance();
        }

    public:
        NativeCounter() : tTJSNativeClass(TJS_W("BridgeCounter")) {
            SetClassID(TJSRegisterNativeClass(TJS_W("BridgeCounter")));
            RegisterNCM(TJS_W("BridgeCounter"),
                        TJSCreateNativeClassMethod(
                            [](tTJSVariant *, tjs_int, tTJSVariant **,
                               iTJSDispatch2 *self) -> tjs_error {
                                return CounterInstance::get(self)
                                    ? TJS_S_OK
                                    : TJS_E_NATIVECLASSCRASH;
                            }),
                        getClassName().c_str(), nitMethod);
            RegisterNCM(TJS_W("readNative"),
                        TJSCreateNativeClassMethod(
                            [](tTJSVariant *result, tjs_int, tTJSVariant **,
                               iTJSDispatch2 *self) -> tjs_error {
                                auto *instance = CounterInstance::get(self);
                                if(!instance)
                                    return TJS_E_NATIVECLASSCRASH;
                                if(result)
                                    *result = instance->value;
                                return TJS_S_OK;
                            }),
                        getClassName().c_str(), nitMethod);
            RegisterNCM(
                TJS_W("value"),
                TJSCreateNativeClassProperty(
                    [](tTJSVariant *result, iTJSDispatch2 *self) -> tjs_error {
                        auto *instance = CounterInstance::get(self);
                        if(!instance)
                            return TJS_E_NATIVECLASSCRASH;
                        if(result)
                            *result = instance->value;
                        return TJS_S_OK;
                    },
                    [](const tTJSVariant *input,
                       iTJSDispatch2 *self) -> tjs_error {
                        auto *instance = CounterInstance::get(self);
                        if(!instance)
                            return TJS_E_NATIVECLASSCRASH;
                        instance->value = tjs_int(*input);
                        return TJS_S_OK;
                    }),
                getClassName().c_str(), nitProperty);
        }
    };
} // namespace

TEST_CASE("Ciallang executes expressions and persistent script globals") {
    Engine engine(new tTJS());
    engine->ExecScript(
        TJS_W("var answer = 40; function plus(v) { return v + answer; }"));
    tTJSVariant result;
    engine->EvalExpression(TJS_W("plus(2)"), &result);
    REQUIRE(tjs_int(result) == 42);
    auto *global = engine->GetGlobalNoAddRef();
    REQUIRE(global->PropGet(0, TJS_W("answer"), nullptr, &result, global) ==
            TJS_S_OK);
    REQUIRE(tjs_int(result) == 40);
}

TEST_CASE("Ciallang preserves the receiver of a registered host closure") {
    Engine engine(new tTJS());
    auto *host = new HostCounter();
    auto *global = engine->GetGlobalNoAddRef();
    tTJSVariant callback(host, host);
    host->Release();
    REQUIRE(global->PropSet(TJS_MEMBERENSURE, TJS_W("counter"), nullptr,
                            &callback, global) == TJS_S_OK);
    tTJSVariant result;
    engine->EvalExpression(TJS_W("counter(41)"), &result);
    REQUIRE(tjs_int(result) == 42);
    REQUIRE(host->calls == 1);
}

TEST_CASE("Ciallang assigns a script function to a host object member") {
    Engine engine(new tTJS());
    auto *host = new tTJSCustomObject();
    auto *global = engine->GetGlobalNoAddRef();
    tTJSVariant hostValue(host, host);
    host->Release();
    REQUIRE(global->PropSet(TJS_MEMBERENSURE, TJS_W("pluginHost"), nullptr,
                            &hostValue, global) == TJS_S_OK);
    engine->ExecScript(
        TJS_W("pluginHost.callback = function (e) { return 1; };"));
    tTJSVariant stored;
    REQUIRE(host->PropGet(0, TJS_W("callback"), nullptr, &stored, host) ==
            TJS_S_OK);
    REQUIRE(stored.Type() == tvtObject);
}

TEST_CASE("Ciallang replaces a script callback on a host native class") {
    Engine engine(new tTJS());
    auto *host = new NativeCounter();
    auto *global = engine->GetGlobalNoAddRef();
    tTJSVariant hostValue(host, host);
    host->Release();
    // System initializes exceptionHandler to null before KAG assigns it.
    tTJSVariant empty(static_cast<iTJSDispatch2 *>(nullptr));
    REQUIRE(host->PropSet(TJS_MEMBERENSURE, TJS_W("callback"), nullptr, &empty,
                          host) == TJS_S_OK);
    REQUIRE(global->PropSet(TJS_MEMBERENSURE, TJS_W("pluginHost"), nullptr,
                            &hostValue, global) == TJS_S_OK);
    engine->ExecScript(
        TJS_W("pluginHost.callback = function (e) { return e + 1; };"));
    tTJSVariant original;
    REQUIRE(host->PropGet(0, TJS_W("callback"), nullptr, &original, host) ==
            TJS_S_OK);
    REQUIRE(original.Type() == tvtObject);
    tTJSVariant argument(tjs_int(40)), result;
    tTJSVariant *args[] = { &argument };
    REQUIRE(original.AsObjectClosureNoAddRef().FuncCall(
                0, nullptr, nullptr, &result, 1, args, host) == TJS_S_OK);
    REQUIRE(tjs_int(result) == 41);

    engine->ExecScript(
        TJS_W("pluginHost.callback = function (e) { return e + 2; };"));
    engine->DoGarbageCollection();
    tTJSVariant replaced;
    REQUIRE(host->PropGet(0, TJS_W("callback"), nullptr, &replaced, host) ==
            TJS_S_OK);
    REQUIRE(replaced.Type() == tvtObject);
    REQUIRE(replaced.AsObjectNoAddRef() != original.AsObjectNoAddRef());
    REQUIRE(replaced.AsObjectClosureNoAddRef().FuncCall(
                0, nullptr, nullptr, &result, 1, args, host) == TJS_S_OK);
    REQUIRE(tjs_int(result) == 42);
}

TEST_CASE("Ciallang assigns native properties from a derived constructor") {
    Engine engine(new tTJS());
    auto *nativeClass = new NativeCounter();
    tTJSVariant type(nativeClass);
    nativeClass->Release();
    auto *global = engine->GetGlobalNoAddRef();
    REQUIRE(global->PropSet(TJS_MEMBERENSURE, TJS_W("BridgeCounter"), nullptr,
                            &type, global) == TJS_S_OK);
    // Mirror YesNoDialogWindow: subclass of a native class assigning a native
    // property (and its own field) inside the constructor.
    engine->ExecScript(TJS_W("class Child extends BridgeCounter {"
                             " var result = false;"
                             " function Child() {"
                             "  super.BridgeCounter();"
                             "  value = 7;"
                             "  result = true;"
                             " }"
                             "} var child = new Child();"));
    tTJSVariant child;
    engine->EvalExpression(TJS_W("child"), &child);
    auto *instance = CounterInstance::get(child.AsObjectNoAddRef());
    REQUIRE(instance);
    REQUIRE(instance->value == 7);
    tTJSVariant flag;
    engine->EvalExpression(TJS_W("child.result"), &flag);
    REQUIRE(tjs_int(flag) != 0);
}

TEST_CASE("Ciallang exposes Array class members to the host") {
    Engine engine(new tTJS());
    // csvParser's InitPlugin does TVPExecuteExpression("Array") then
    // PropGet("clear").
    tTJSVariant arrayClass;
    engine->EvalExpression(TJS_W("Array"), &arrayClass);
    REQUIRE(arrayClass.Type() == tvtObject);
    auto *dispatch = arrayClass.AsObjectNoAddRef();
    REQUIRE(dispatch != nullptr);
    tTJSVariant clear;
    const auto status = dispatch->PropGet(TJS_IGNOREPROP, TJS_W("clear"),
                                          nullptr, &clear, dispatch);
    REQUIRE(status == TJS_S_OK);
    REQUIRE(clear.Type() == tvtObject);

    engine->ExecScript(TJS_W("var items = [1, 2, 3]; var other = [4, 5];"));
    tTJSVariant items, other, result;
    engine->EvalExpression(TJS_W("items"), &items);
    engine->EvalExpression(TJS_W("other"), &other);
    // Plugins retain the method dispatch and supply the target as objthis.
    REQUIRE(clear.AsObjectNoAddRef()->FuncCall(
                0, nullptr, nullptr, nullptr, 0, nullptr,
                items.AsObjectNoAddRef()) == TJS_S_OK);
    engine->EvalExpression(TJS_W("items.count"), &result);
    REQUIRE(tjs_int(result) == 0);
    engine->EvalExpression(TJS_W("other.count"), &result);
    REQUIRE(tjs_int(result) == 2);

    // An instance member read produces a closure retaining that instance.
    auto *dispatchOther = other.AsObjectNoAddRef();
    tTJSVariant bound;
    REQUIRE(dispatchOther->PropGet(TJS_IGNOREPROP, TJS_W("clear"), nullptr,
                                   &bound, dispatchOther) == TJS_S_OK);
    other.Clear();
    engine->DoGarbageCollection();
    REQUIRE(bound.AsObjectClosureNoAddRef().FuncCall(
                0, nullptr, nullptr, nullptr, 0, nullptr, nullptr) == TJS_S_OK);
    engine->EvalExpression(TJS_W("other.count"), &result);
    REQUIRE(tjs_int(result) == 0);
}

TEST_CASE("Ciallang resolves native members on bridged instances") {
    Engine engine(new tTJS());
    tTJSVariant array, result;
    engine->EvalExpression(TJS_W("[1, 2]"), &array);
    auto *dispatch = array.AsObjectNoAddRef();
    REQUIRE(dispatch->PropGet(0, TJS_W("count"), nullptr, &result, dispatch) ==
            TJS_S_OK);
    REQUIRE(tjs_int(result) == 2);
    REQUIRE(dispatch->FuncCall(0, TJS_W("clear"), nullptr, nullptr, 0, nullptr,
                               dispatch) == TJS_S_OK);
    REQUIRE(dispatch->PropGet(0, TJS_W("count"), nullptr, &result, dispatch) ==
            TJS_S_OK);
    REQUIRE(tjs_int(result) == 0);
    REQUIRE(dispatch->PropGet(0, TJS_W("absentMember"), nullptr, &result,
                              dispatch) == TJS_E_MEMBERNOTFOUND);
    engine->EvalExpression(TJS_W("typeof [1].absentMember"), &result);
    REQUIRE(ttstr(result) == ttstr(TJS_W("undefined")));

    // Dictionary methods use a different native type table.
    tTJSVariant dictionary, clear;
    engine->EvalExpression(TJS_W("%[answer: 42]"), &dictionary);
    dispatch = dictionary.AsObjectNoAddRef();
    REQUIRE(dispatch->PropGet(0, TJS_W("clear"), nullptr, &clear, dispatch) ==
            TJS_S_OK);
    REQUIRE(clear.AsObjectClosureNoAddRef().FuncCall(
                0, nullptr, nullptr, nullptr, 0, nullptr, nullptr) == TJS_S_OK);
    REQUIRE(dispatch->PropGet(0, TJS_W("answer"), nullptr, &result, dispatch) ==
            TJS_E_MEMBERNOTFOUND);
}

TEST_CASE("Ciallang probes a missing host member as undefined") {
    Engine engine(new tTJS());
    auto *host = new HostCounter();
    auto *global = engine->GetGlobalNoAddRef();
    tTJSVariant hostValue(host, host);
    host->Release();
    REQUIRE(global->PropSet(TJS_MEMBERENSURE, TJS_W("pluginHost"), nullptr,
                            &hostValue, global) == TJS_S_OK);
    // KAG probes native class members this way before linking a plugin.
    tTJSVariant result;
    engine->EvalExpression(TJS_W("typeof pluginHost.absentMember"), &result);
    REQUIRE(ttstr(result) == ttstr(TJS_W("undefined")));
}

TEST_CASE("Ciallang keeps exported callbacks alive across collection") {
    Engine engine(new tTJS());
    tTJSVariant callback;
    engine->EvalExpression(TJS_W("function(v) { return v * 2; }"), &callback);
    engine->DoGarbageCollection();
    tTJSVariant argument(tjs_int(21)), result;
    tTJSVariant *args[] = { &argument };
    auto closure = callback.AsObjectClosureNoAddRef();
    REQUIRE(closure.FuncCall(0, nullptr, nullptr, &result, 1, args, nullptr) ==
            TJS_S_OK);
    REQUIRE(tjs_int(result) == 42);
}

TEST_CASE("Ciallang invalidates retained callbacks at runtime shutdown") {
    Engine engine(new tTJS());
    tTJSVariant callback;
    engine->EvalExpression(TJS_W("function() { return 42; }"), &callback);
    engine.reset();
    tTJSVariant result;
    auto closure = callback.AsObjectClosureNoAddRef();
    REQUIRE(closure.FuncCall(0, nullptr, nullptr, &result, 0, nullptr,
                             nullptr) == TJS_E_INVALIDOBJECT);
}

TEST_CASE("Ciallang proxy release preserves a live script object") {
    Engine engine(new tTJS());
    engine->ExecScript(TJS_W("var finalized = 0;"
                             "class C { function C() {} function finalize() { "
                             "global.finalized += 1; } }"
                             "var kept = new C();"));
    tTJSVariant temporary;
    engine->EvalExpression(TJS_W("kept"), &temporary);
    temporary.Clear();
    engine->DoGarbageCollection();
    tTJSVariant result;
    engine->EvalExpression(TJS_W("finalized"), &result);
    REQUIRE(tjs_int(result) == 0);
}


TEST_CASE("Ciallang initializes script fields on a native base instance") {
    Engine engine(new tTJS());
    auto *nativeClass = new NativeCounter();
    tTJSVariant type(nativeClass);
    nativeClass->Release();
    auto *global = engine->GetGlobalNoAddRef();
    REQUIRE(global->PropSet(TJS_MEMBERENSURE, TJS_W("BridgeCounter"), nullptr,
                            &type, global) == TJS_S_OK);
    // KAGWindow declares many initialized fields (var scWidth = 640; ...) above
    // a constructor that calls super.Window().
    engine->ExecScript(TJS_W("class Child extends BridgeCounter {"
                             " var width = 640;"
                             " var height = 480;"
                             " var name = \"kag\";"
                             " function Child() { super.BridgeCounter(); }"
                             "} var child = new Child();"));
    tTJSVariant result;
    engine->EvalExpression(TJS_W("child.width"), &result);
    REQUIRE(tjs_int(result) == 640);
    engine->EvalExpression(TJS_W("child.name"), &result);
    REQUIRE(ttstr(result) == ttstr(TJS_W("kag")));
    engine->EvalExpression(TJS_W("child"), &result);
    REQUIRE(CounterInstance::get(result.AsObjectNoAddRef()) != nullptr);
}

TEST_CASE("Ciallang script inheritance preserves the actual native instance") {
    Engine engine(new tTJS());
    auto *nativeClass = new NativeCounter();
    tTJSVariant type(nativeClass);
    nativeClass->Release();
    auto *global = engine->GetGlobalNoAddRef();
    REQUIRE(global->PropSet(TJS_MEMBERENSURE, TJS_W("BridgeCounter"), nullptr,
                            &type, global) == TJS_S_OK);
    engine->ExecScript(TJS_W("class Child extends BridgeCounter {"
                             " function Child() { super.BridgeCounter(); }"
                             " function bump() { return ++value; }"
                             "} var child = new Child();"));
    tTJSVariant result;
    engine->EvalExpression(TJS_W("child.bump()"), &result);
    REQUIRE(tjs_int(result) == 42);
    tTJSVariant child;
    engine->EvalExpression(TJS_W("child"), &child);
    auto *instance = CounterInstance::get(child.AsObjectNoAddRef());
    REQUIRE(instance);
    REQUIRE(instance->value == 42);
    REQUIRE(child.AsObjectNoAddRef()->FuncCall(
                0, TJS_W("readNative"), nullptr, &result, 0, nullptr,
                child.AsObjectNoAddRef()) == TJS_S_OK);
    REQUIRE(tjs_int(result) == 42);
    engine->DoGarbageCollection();
    engine->EvalExpression(TJS_W("child.value"), &result);
    REQUIRE(tjs_int(result) == 42);
    engine->EvalExpression(TJS_W("child instanceof \"BridgeCounter\""),
                           &result);
    REQUIRE(tjs_int(result) == 1);
}


TEST_CASE(
    "Ciallang exceptions own valid source metadata across the host boundary") {
    Engine engine(new tTJS());
    try {
        engine->ExecScript(TJS_W("throw 123;"), nullptr, nullptr,
                           TJS_W("xp3://scripts/throw.tjs"), 7);
        FAIL("The thrown value must cross the host boundary");
    } catch(eTJSScriptException &error) {
        REQUIRE(tjs_int(error.GetValue()) == 123);
        REQUIRE(ttstr(error.GetBlockName()) ==
                ttstr(TJS_W("xp3://scripts/throw.tjs")));
        REQUIRE(error.GetSourceLine() == 8);
        tjs_int length = 0;
        const auto *line = error.GetBlockNoAddRef()->GetLine(7, &length);
        REQUIRE(ttstr(line, length) == ttstr(TJS_W("throw 123;")));
    }
}

TEST_CASE(
    "Ciallang preserves missing member probes and nested class construction") {
    Engine engine(new tTJS());
    engine->ExecScript(TJS_W(
        "class Registry {"
        " var map = %[];"
        " class Alias { var target; function Alias(t) { target = t; } }"
        " function Registry() {}"
        " function set(t,v) { map[t] = v; }"
        " function alias(t,to) { map[t] = new Alias(to); }"
        " function get(t) { while(map[t] instanceof \"Alias\") t = "
        "map[t].target; return map[t]; }"
        " function defined(t) { return typeof map[t] !== \"undefined\"; }"
        "}"
        "class Hooks extends Registry {"
        " function Hooks() { super.Registry(); }"
        " function add(t,f) { set(t, []) if(!defined(t)); get(t).add(f); }"
        "}"
        "var hooks = new Hooks(); hooks.add(\"start\", 7);"
        "hooks.alias(\"alias\", \"start\");"));
    tTJSVariant result;
    engine->EvalExpression(TJS_W("hooks.get(\"alias\")[0]"), &result);
    REQUIRE(tjs_int(result) == 7);
    engine->EvalExpression(TJS_W("typeof global.__krkrMissingProbe"), &result);
    REQUIRE(ttstr(result) == ttstr(TJS_W("undefined")));
}

TEST_CASE("Ciallang arrays use host text storage paths modes and encoding") {
    struct Streams {
        ttstr input = TJS_W("一\r\n\r\n二\nlast");
        ttstr output, name, mode;
        int closed = 0;
        bool fail = false;
    } streams;
    static Streams *active;
    active = &streams;
    const auto oldRead = TJSCreateTextStreamForRead;
    const auto oldWrite = TJSCreateTextStreamForWrite;
    struct Restore {
        decltype(oldRead) read;
        decltype(oldWrite) write;
        ~Restore() {
            TJSCreateTextStreamForRead = read;
            TJSCreateTextStreamForWrite = write;
        }
    } restore{ oldRead, oldWrite };
    struct Reader : iTJSTextReadStream {
        tjs_uint Read(ttstr &value, tjs_uint) override {
            value = active->input;
            return value.GetLen();
        }
        void Destruct() override {
            ++active->closed;
            delete this;
        }
    };
    struct Writer : iTJSTextWriteStream {
        void Write(const ttstr &value) override {
            if(active->fail)
                throw std::runtime_error("write failed");
            active->output += value;
        }
        void Destruct() override {
            ++active->closed;
            delete this;
        }
    };
    TJSCreateTextStreamForRead = [](const ttstr &name,
                                    const ttstr &mode) -> iTJSTextReadStream * {
        active->name = name;
        active->mode = mode;
        return new Reader();
    };
    TJSCreateTextStreamForWrite =
        [](const ttstr &name, const ttstr &mode) -> iTJSTextWriteStream * {
        active->name = name;
        active->mode = mode;
        return new Writer();
    };
    Engine engine(new tTJS());
    engine->ExecScript(
        TJS_W("var lines = [\"中文\", 7, 1.25, void, %[]];"
              "var saved = lines.save(\"file://./virtual/log\", \"z1\");"));
    REQUIRE(streams.name == ttstr(TJS_W("file://./virtual/log")));
    REQUIRE(streams.mode == ttstr(TJS_W("z1")));
    REQUIRE(streams.closed == 1);
    const auto scriptOutput = streams.output;
    streams.output = TJS_W("");
    auto *reference = TJSCreateArrayObject();
    tTJSVariant nativeArray(reference, reference);
    reference->Release();
    tTJSVariant values[] = { tTJSVariant(TJS_W("中文")), tTJSVariant(7),
                             tTJSVariant(1.25), tTJSVariant(), nativeArray };
    for(int i = 0; i < 5; ++i)
        REQUIRE(reference->PropSetByNum(TJS_MEMBERENSURE, i, &values[i],
                                        reference) == TJS_S_OK);
    tTJSVariant name(TJS_W("file://./virtual/log")), mode(TJS_W("z1"));
    tTJSVariant *args[] = { &name, &mode };
    REQUIRE(reference->FuncCall(0, TJS_W("save"), nullptr, nullptr, 2, args,
                                reference) == TJS_S_OK);
    REQUIRE(streams.output == scriptOutput);
    // Avoid retaining a self-cycle in the native reference array.
    reference->FuncCall(0, TJS_W("clear"), nullptr, nullptr, 0, nullptr,
                        reference);
    engine->ExecScript(TJS_W(
        "var loaded = lines.load(\"archive.xp3>lines.txt\", \"utf-8\");"));
    REQUIRE(streams.name == ttstr(TJS_W("archive.xp3>lines.txt")));
    REQUIRE(streams.mode == ttstr(TJS_W("utf-8")));
    tTJSVariant result;
    engine->EvalExpression(TJS_W("(saved === lines) && (loaded === lines)"),
                           &result);
    REQUIRE(tjs_int(result) == 1);
    engine->EvalExpression(TJS_W("lines.join(\"|\")"), &result);
    REQUIRE(ttstr(result) == ttstr(TJS_W("一||二|last")));
    const auto beforeFailure = streams.closed;
    streams.fail = true;
    engine->ExecScript(TJS_W(
        "var writeCaught = false; try { lines.save(\"file://./failure\"); }"
        "catch(e) { writeCaught = true; }"));
    engine->EvalExpression(TJS_W("writeCaught"), &result);
    REQUIRE(tjs_int(result) == 1);
    REQUIRE(streams.closed == beforeFailure + 1);
}

TEST_CASE(
    "Ciallang default constructor arguments preserve caller local values") {
    Engine engine(new tTJS());
    engine->ExecScript(
        TJS_W("class Defaulted { function Defaulted(a=1,b=2,c=3,d=4) {} }"
              "function constructTwice() { var kept = \"sentinel\";"
              "new Defaulted(); new Defaulted(); return kept; }"));
    tTJSVariant result;
    engine->EvalExpression(TJS_W("constructTwice()"), &result);
    REQUIRE(ttstr(result) == ttstr(TJS_W("sentinel")));
}

TEST_CASE(
    "Ciallang exposes inherited script methods with their lexical superclass") {
    Engine engine(new tTJS());
    engine->ExecScript(TJS_W(
        "class Root { var n = 0; function Root() { n = 3; }"
        "function set(v) { n = v; } function read() { return n; } }"
        "class Middle extends Root { function Middle() { super.Root(); }"
        "function read() { return super.read() + 1; } }"
        "class Leaf extends Middle { function Leaf() { super.Middle(); set(5); "
        "}"
        "function total() { return read() + 1; } } var leaf = new Leaf();"));
    tTJSVariant result, klass, leaf;
    engine->EvalExpression(TJS_W("leaf.total()"), &result);
    REQUIRE(tjs_int(result) == 7);
    engine->EvalExpression(TJS_W("Leaf"), &klass);
    engine->EvalExpression(TJS_W("leaf"), &leaf);
    auto *dispatch = klass.AsObjectNoAddRef();
    REQUIRE(dispatch->PropGet(0, TJS_W("set"), nullptr, &result, dispatch) ==
            TJS_S_OK);
    REQUIRE(result.Type() == tvtObject);
    tTJSVariant value(9);
    tTJSVariant *args[] = { &value };
    REQUIRE(dispatch->FuncCall(0, TJS_W("set"), nullptr, nullptr, 1, args,
                               leaf.AsObjectNoAddRef()) == TJS_S_OK);
    engine->EvalExpression(TJS_W("leaf.total()"), &result);
    REQUIRE(tjs_int(result) == 11);
}
