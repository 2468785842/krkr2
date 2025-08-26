#pragma once

namespace SimpleBinder {

    namespace Detail {

        //--------------------------------------------------------------
        // unroll macro utils

#define UNROLL_AM_0(_A_, _M_) (_A_)
#define UNROLL_AM_1(_A_, _M_) (_A_, _M_(0))
#define UNROLL_AM_2(_A_, _M_) (_A_, _M_(0), _M_(1))
#define UNROLL_AM_3(_A_, _M_) (_A_, _M_(0), _M_(1), _M_(2))
#define UNROLL_AM_4(_A_, _M_) (_A_, _M_(0), _M_(1), _M_(2), _M_(3))
#define UNROLL_AM_5(_A_, _M_) (_A_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4))
#define UNROLL_AM_6(_A_, _M_)                                                  \
    (_A_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4), _M_(5))
#define UNROLL_AM_7(_A_, _M_)                                                  \
    (_A_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4), _M_(5), _M_(6))
#define UNROLL_AM_8(_A_, _M_)                                                  \
    (_A_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4), _M_(5), _M_(6), _M_(7))

#define UNROLL_ABM_0(_A_, _B_, _M_) (_A_, _B_)
#define UNROLL_ABM_1(_A_, _B_, _M_) (_A_, _B_, _M_(0))
#define UNROLL_ABM_2(_A_, _B_, _M_) (_A_, _B_, _M_(0), _M_(1))
#define UNROLL_ABM_3(_A_, _B_, _M_) (_A_, _B_, _M_(0), _M_(1), _M_(2))
#define UNROLL_ABM_4(_A_, _B_, _M_) (_A_, _B_, _M_(0), _M_(1), _M_(2), _M_(3))
#define UNROLL_ABM_5(_A_, _B_, _M_)                                            \
    (_A_, _B_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4))
#define UNROLL_ABM_6(_A_, _B_, _M_)                                            \
    (_A_, _B_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4), _M_(5))
#define UNROLL_ABM_7(_A_, _B_, _M_)                                            \
    (_A_, _B_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4), _M_(5), _M_(6))
#define UNROLL_ABM_8(_A_, _B_, _M_)                                            \
    (_A_, _B_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4), _M_(5), _M_(6), _M_(7))

#define UNROLL_AMYZ_0(_A_, _M_, _Y_, _Z_) (_A_, _Y_, _Z_)
#define UNROLL_AMYZ_1(_A_, _M_, _Y_, _Z_) (_A_, _M_(0), _Y_, _Z_)
#define UNROLL_AMYZ_2(_A_, _M_, _Y_, _Z_) (_A_, _M_(0), _M_(1), _Y_, _Z_)
#define UNROLL_AMYZ_3(_A_, _M_, _Y_, _Z_)                                      \
    (_A_, _M_(0), _M_(1), _M_(2), _Y_, _Z_)
#define UNROLL_AMYZ_4(_A_, _M_, _Y_, _Z_)                                      \
    (_A_, _M_(0), _M_(1), _M_(2), _M_(3), _Y_, _Z_)
#define UNROLL_AMYZ_5(_A_, _M_, _Y_, _Z_)                                      \
    (_A_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4), _Y_, _Z_)
#define UNROLL_AMYZ_6(_A_, _M_, _Y_, _Z_)                                      \
    (_A_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4), _M_(5), _Y_, _Z_)
#define UNROLL_AMYZ_7(_A_, _M_, _Y_, _Z_)                                      \
    (_A_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4), _M_(5), _M_(6), _Y_, _Z_)
#define UNROLL_AMYZ_8(_A_, _M_, _Y_, _Z_)                                      \
    (_A_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4), _M_(5), _M_(6), _M_(7), _Y_, \
     _Z_)

#define UNROLL_ABMYZ_0(_A_, _B_, _M_, _Y_, _Z_) (_A_, _B_, _Y_, _Z_)
#define UNROLL_ABMYZ_1(_A_, _B_, _M_, _Y_, _Z_) (_A_, _B_, _M_(0), _Y_, _Z_)
#define UNROLL_ABMYZ_2(_A_, _B_, _M_, _Y_, _Z_)                                \
    (_A_, _B_, _M_(0), _M_(1), _Y_, _Z_)
#define UNROLL_ABMYZ_3(_A_, _B_, _M_, _Y_, _Z_)                                \
    (_A_, _B_, _M_(0), _M_(1), _M_(2), _Y_, _Z_)
#define UNROLL_ABMYZ_4(_A_, _B_, _M_, _Y_, _Z_)                                \
    (_A_, _B_, _M_(0), _M_(1), _M_(2), _M_(3), _Y_, _Z_)
#define UNROLL_ABMYZ_5(_A_, _B_, _M_, _Y_, _Z_)                                \
    (_A_, _B_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4), _Y_, _Z_)
#define UNROLL_ABMYZ_6(_A_, _B_, _M_, _Y_, _Z_)                                \
    (_A_, _B_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4), _M_(5), _Y_, _Z_)
#define UNROLL_ABMYZ_7(_A_, _B_, _M_, _Y_, _Z_)                                \
    (_A_, _B_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4), _M_(5), _M_(6), _Y_, _Z_)
#define UNROLL_ABMYZ_8(_A_, _B_, _M_, _Y_, _Z_)                                \
    (_A_, _B_, _M_(0), _M_(1), _M_(2), _M_(3), _M_(4), _M_(5), _M_(6), _M_(7), \
     _Y_, _Z_)

#define UNROLL_EVAL_0(_M_) _M_
#define UNROLL_EVAL(_A_, _B_, _C_) UNROLL_EVAL_0(_A_##_B_ _C_)
#define UNROLL_FOREACH(_M1_, _M2_, _A1_, _A2_)                                 \
    _M1_(0, UNROLL_EVAL(_M2_, _0, _A1_), UNROLL_EVAL(_M2_, _0, _A2_));         \
    _M1_(1, UNROLL_EVAL(_M2_, _1, _A1_), UNROLL_EVAL(_M2_, _1, _A2_));         \
    _M1_(2, UNROLL_EVAL(_M2_, _2, _A1_), UNROLL_EVAL(_M2_, _2, _A2_));         \
    _M1_(3, UNROLL_EVAL(_M2_, _3, _A1_), UNROLL_EVAL(_M2_, _3, _A2_));         \
    _M1_(4, UNROLL_EVAL(_M2_, _4, _A1_), UNROLL_EVAL(_M2_, _4, _A2_));         \
    _M1_(5, UNROLL_EVAL(_M2_, _5, _A1_), UNROLL_EVAL(_M2_, _5, _A2_));         \
    _M1_(6, UNROLL_EVAL(_M2_, _6, _A1_), UNROLL_EVAL(_M2_, _6, _A2_));         \
    _M1_(7, UNROLL_EVAL(_M2_, _7, _A1_), UNROLL_EVAL(_M2_, _7, _A2_));         \
    _M1_(8, UNROLL_EVAL(_M2_, _8, _A1_), UNROLL_EVAL(_M2_, _8, _A2_))

        //--------------------------------------------------------------
        // Multi args callback template specializations.

        // util class
        template <class C>
        struct ClassInstanceResolver;

        struct MethodInvokerBase {
            typedef tjs_int Num;
            typedef iTJSDispatch2 *Dp;
            typedef tTJSVariant *Vp;
            static inline Num VarNum(Num numparams, Num sub) {
                Num r = numparams - sub;
                return r < 0 ? 0 : r;
            }
            static inline Vp *VarArgs(Num numparams, Num sub, Vp *param) {
                return numparams <= sub ? 0 : param + sub;
            }
            template <class C>
            static bool GetSelf(Dp objthis, C *&ptr) {
                typedef typename ClassInstanceResolver<C>::Wrapper Wrapper;
                ptr = Wrapper::GetInstance(objthis);
                return ptr != 0;
            }
        };

        // util macro

#define ARGTYPE(_N_) tTJSVariant *
#define ARGREF(_N_) param[_N_]
#define VARGNUM VarNum(numparams, Count)
#define VARGREF VarArgs(numparams, Count, param)

        ////============================
        //// factory invoker

        template <typename FUNC>
        struct FactoryInvoker;

#define DEF_SCF(CNT, ARGS, EXT)                                                \
    template <class C>                                                         \
    struct FactoryInvoker<tjs_error(*) ARGS> : public MethodInvokerBase {      \
        enum { Count = CNT };                                                  \
        typedef tjs_error(*Func) ARGS;                                         \
        typedef C Class;                                                       \
        typedef C *&Inst;                                                      \
        static inline tjs_error Invoke(const Func &func, Inst result,          \
                                       Num numparams, Vp *param,               \
                                       iTJSDispatch2 *objthis) {               \
            return func EXT;                                                   \
        }                                                                      \
    }

        // factory(inst, args*)
        UNROLL_FOREACH(DEF_SCF, UNROLL_AM, (C *&, ARGTYPE), (result, ARGREF));

        // factory(inst, args*, optnum, optargs)
        UNROLL_FOREACH(DEF_SCF, UNROLL_AMYZ,
                       (C *&, ARGTYPE, tjs_int, tTJSVariant **),
                       (result, ARGREF, VARGNUM, VARGREF));

        // factory(objthis, inst, args*)
        UNROLL_FOREACH(DEF_SCF, UNROLL_ABM, (iTJSDispatch2 *, C *&, ARGTYPE),
                       (objthis, result, ARGREF));

        // factory(objthis, inst, args*, optnum, optargs)
        UNROLL_FOREACH(DEF_SCF, UNROLL_ABMYZ,
                       (iTJSDispatch2 *, C *&, ARGTYPE, tjs_int,
                        tTJSVariant **),
                       (objthis, result, ARGREF, VARGNUM, VARGREF));


        ////============================
        //// method invoker

        template <typename FUNC>
        struct MethodInvoker;

#define DEF_SMI(CNT, ARGS, EXT, STATIC)                                        \
    template <>                                                                \
    struct MethodInvoker<tjs_error(*) ARGS> : public MethodInvokerBase {       \
        enum { Count = CNT, Static = STATIC };                                 \
        typedef tjs_error(*Func) ARGS;                                         \
        static inline tjs_error Invoke(const Func &func, Vp result,            \
                                       Num numparams, Vp *param, Dp objthis) { \
            return func EXT;                                                   \
        }                                                                      \
    }

#define DEF_SMI_S(CNT, ARGS, EXT) DEF_SMI(CNT, ARGS, EXT, 1)
#define DEF_SMI_C(CNT, ARGS, EXT) DEF_SMI(CNT, ARGS, EXT, 0)

        // method(result, args*)
        UNROLL_FOREACH(DEF_SMI_S, UNROLL_AM, (tTJSVariant *, ARGTYPE),
                       (result, ARGREF));

        // method(result, args*, optnum, optargs)
        UNROLL_FOREACH(DEF_SMI_S, UNROLL_AMYZ,
                       (tTJSVariant *, ARGTYPE, tjs_int, tTJSVariant **),
                       (result, ARGREF, VARGNUM, VARGREF));

        // method(objthis, result, args*)
        UNROLL_FOREACH(DEF_SMI_C, UNROLL_ABM,
                       (iTJSDispatch2 *, tTJSVariant *, ARGTYPE),
                       (objthis, result, ARGREF));

        // method(objthis, result, args*, optnum, optargs)
        UNROLL_FOREACH(DEF_SMI_C, UNROLL_ABMYZ,
                       (iTJSDispatch2 *, tTJSVariant *, ARGTYPE, tjs_int,
                        tTJSVariant **),
                       (objthis, result, ARGREF, VARGNUM, VARGREF));


#define DEF_SCM(CNT, ARGS, EXT)                                                \
    template <class C>                                                         \
    struct MethodInvoker<tjs_error(C::*) ARGS> : public MethodInvokerBase {    \
        enum { Count = CNT, Static = 0 };                                      \
        typedef tjs_error(C::*Func) ARGS;                                      \
        static inline tjs_error Invoke(const Func &func, Vp result,            \
                                       Num numparams, Vp *param, Dp objthis) { \
            C *self = 0;                                                       \
            return (objthis && GetSelf(objthis, self))                         \
                ? ((self->*func) EXT)                                          \
                : TJS_E_NATIVECLASSCRASH;                                      \
        }                                                                      \
    }

        // instance->method(result, args*)
        UNROLL_FOREACH(DEF_SCM, UNROLL_AM, (tTJSVariant *, ARGTYPE),
                       (result, ARGREF));

        // instance->method(result, args*, optnum, optargs)
        UNROLL_FOREACH(DEF_SCM, UNROLL_AMYZ,
                       (tTJSVariant *, ARGTYPE, tjs_int, tTJSVariant **),
                       (result, ARGREF, VARGNUM, VARGREF));

#define DEF_SCS(CNT, ARGS, EXT)                                                \
    template <class C>                                                         \
    struct MethodInvoker<tjs_error(*) ARGS> : public MethodInvokerBase {       \
        enum { Count = CNT, Static = 0 };                                      \
        typedef tjs_error(*Func) ARGS;                                         \
        static inline tjs_error Invoke(const Func &func, Vp result,            \
                                       Num numparams, Vp *param, Dp objthis) { \
            C *self = 0;                                                       \
            return (objthis && GetSelf(objthis, self))                         \
                ? (func EXT)                                                   \
                : TJS_E_NATIVECLASSCRASH;                                      \
        }                                                                      \
    }

        // method(instance, result, args*)
        UNROLL_FOREACH(DEF_SCS, UNROLL_ABM, (C *, tTJSVariant *, ARGTYPE),
                       (self, result, ARGREF));

        // method(instance, result, args*, optnum, optargs)
        UNROLL_FOREACH(DEF_SCS, UNROLL_ABMYZ,
                       (C *, tTJSVariant *, ARGTYPE, tjs_int, tTJSVariant **),
                       (self, result, ARGREF, VARGNUM, VARGREF));


        //--------------------------------------------------------------
        // undef macro

#undef UNROLL_AM_0
#undef UNROLL_AM_1
#undef UNROLL_AM_2
#undef UNROLL_AM_3
#undef UNROLL_AM_4
#undef UNROLL_AM_5
#undef UNROLL_AM_6
#undef UNROLL_AM_7
#undef UNROLL_AM_8

#undef UNROLL_ABM_0
#undef UNROLL_ABM_1
#undef UNROLL_ABM_2
#undef UNROLL_ABM_3
#undef UNROLL_ABM_4
#undef UNROLL_ABM_5
#undef UNROLL_ABM_6
#undef UNROLL_ABM_7
#undef UNROLL_ABM_8

#undef UNROLL_AMYZ_0
#undef UNROLL_AMYZ_1
#undef UNROLL_AMYZ_2
#undef UNROLL_AMYZ_3
#undef UNROLL_AMYZ_4
#undef UNROLL_AMYZ_5
#undef UNROLL_AMYZ_6
#undef UNROLL_AMYZ_7
#undef UNROLL_AMYZ_8

#undef UNROLL_ABMYZ_0
#undef UNROLL_ABMYZ_1
#undef UNROLL_ABMYZ_2
#undef UNROLL_ABMYZ_3
#undef UNROLL_ABMYZ_4
#undef UNROLL_ABMYZ_5
#undef UNROLL_ABMYZ_6
#undef UNROLL_ABMYZ_7
#undef UNROLL_ABMYZ_8

#undef UNROLL_EVAL
#undef UNROLL_FOREACH


#undef ARGTYPE
#undef ARGREF
#undef VARGNUM
#undef VARGREF

#undef DEF_SMI
#undef DEF_SMI_S
#undef DEF_SMI_C
#undef DEF_SCM
#undef DEF_SCS


        //--------------------------------------------------------------
        // exception handle trapper

        template <class TRAP, typename FUNC>
        struct ExceptionTrapper {
            typedef typename TRAP::Exception Exception;
            typedef typename TRAP::Message Message;
            typedef typename TRAP::FuncName FuncName;

            ExceptionTrapper(const FUNC &func, const FuncName &name) :
                func(func), funcname(name) {}
            virtual ~ExceptionTrapper() {}

            tjs_error invoke(MethodInvokerBase::Vp result,
                             MethodInvokerBase::Num numparams,
                             MethodInvokerBase::Vp *param,
                             MethodInvokerBase::Dp objthis) const {
                Message message;
                try {
                    return MethodInvoker<FUNC>::Invoke(func, result, numparams,
                                                       param, objthis);
                } catch(const Exception &exception) {
                    if(!TRAP::Catch(exception, message))
                        throw;
                }
                return TRAP::Trap(funcname, message);
            }
            template <class FACTORY>
            tjs_error invoke(FACTORY *&result, MethodInvokerBase::Num numparams,
                             MethodInvokerBase::Vp *param,
                             MethodInvokerBase::Dp objthis) const {
                Message message;
                try {
                    return FactoryInvoker<FUNC>::Invoke(func, result, numparams,
                                                        param, objthis);
                } catch(const Exception &exception) {
                    if(!TRAP::Catch(exception, message))
                        throw;
                }
                return TRAP::Trap(funcname, message);
            }

        protected:
            const FUNC func;
            const FuncName funcname;
        };

        template <class EXCEPTION>
        struct ExceptionRethrowTrap {
            typedef ExceptionRethrowTrap<EXCEPTION> SelfType;
            typedef EXCEPTION Exception;
            typedef ttstr *Message;
            typedef const tjs_char *FuncName;

            static bool Catch(const Exception &exception, Message &message) {
                static ttstr error; // [XXX] temporary textwork
                error = ttstr(exception.what());
                message = &error;
                return true; // true:call Trap,  false:throw
            }
            static tjs_error Trap(const FuncName name, const Message message) {
                if(name)
                    TVPThrowExceptionMessage(message->c_str(), name);
                else
                    TVPThrowExceptionMessage(message->c_str());
                return TJS_E_FAIL;
            }

            template <typename FUNC>
            static ExceptionTrapper<SelfType, FUNC>
            Get(const FUNC &func, const FuncName name = 0) {
                return ExceptionTrapper<SelfType, FUNC>(func, name);
            }
        };

        template <class TARGET, int HAS_STATIC>
        struct PassThroughInvokerRefStatic {
            enum { Static = 0 };
        };
        template <class TARGET>
        struct PassThroughInvokerRefStatic<TARGET, 1> {
            enum { Static = TARGET::Static };
        };

        template <class PASS, class TARGET, int HAS_STATIC>
        struct PassThroughInvoker : public MethodInvokerBase {
            enum {
                Count = TARGET::Count,
                Static = PassThroughInvokerRefStatic<TARGET, HAS_STATIC>::Static
            };
            static inline tjs_error Invoke(const PASS &pass, Vp result,
                                           Num numparams, Vp *param,
                                           Dp objthis) {
                return pass.invoke(result, numparams, param, objthis);
            }
            template <class FACTORY>
            static inline tjs_error Invoke(const PASS &pass, FACTORY *&result,
                                           Num numparams, Vp *param,
                                           iTJSDispatch2 *objthis) {
                return pass.invoke(result, numparams, param, objthis);
            }
        };
        template <class TRAP, typename FUNC>
        struct MethodInvoker<ExceptionTrapper<TRAP, FUNC>>
            : public PassThroughInvoker<ExceptionTrapper<TRAP, FUNC>,
                                        MethodInvoker<FUNC>, 1> {};

        template <class TRAP, typename FUNC>
        struct FactoryInvoker<ExceptionTrapper<TRAP, FUNC>>
            : public PassThroughInvoker<ExceptionTrapper<TRAP, FUNC>,
                                        FactoryInvoker<FUNC>, 0> {};


        //--------------------------------------------------------------
        // TJS Store util

        class StoreUtil : public tTJSDispatch {
        public:
            static const tjs_uint32 DEFAULT_FLAG =
                TJS_MEMBERENSURE | TJS_IGNOREPROP;
            StoreUtil() : _flag(DEFAULT_FLAG) {}
            StoreUtil(tjs_uint32 flag) : _flag(flag) {}

        protected:
            tjs_uint32 _flag;
            bool store(iTJSDispatch2 *obj, const ttstr &key,
                       iTJSDispatch2 *context = NULL) {
                tTJSVariant val(static_cast<iTJSDispatch2 *>(this), context);
                this->Release();
                return obj &&
                    TJS_SUCCEEDED(
                           obj->PropSet(_flag, key.c_str(), NULL, &val, obj));
            }
            static inline void Error(const ttstr &message) {
                TVPAddImportantLog(message);
            }
            virtual bool checkInstanceOf(const tjs_char *classname) {
                return false;
            }
            inline static bool compareString(const tjs_char *s1,
                                             const tjs_char *s2) {
                return !TJS_stricmp(s1, s2); // [XXX] ignore case compare
            }

        public:
            tjs_error TJS_INTF_METHOD IsInstanceOf(tjs_uint32 flag,
                                                   const tjs_char *membername,
                                                   tjs_uint32 *hint,
                                                   const tjs_char *classname,
                                                   iTJSDispatch2 *objthis) {
                if(!membername) {
                    return (checkInstanceOf(classname) ||
                            compareString(classname, TJS_W("Object")))
                        ? TJS_S_TRUE
                        : TJS_S_FALSE;
                }
                return tTJSDispatch::IsInstanceOf(flag, membername, hint,
                                                  classname, objthis);
            }

            static inline bool Link(iTJSDispatch2 *obj, const ttstr &key,
                                    StoreUtil *self,
                                    iTJSDispatch2 *context = NULL) {
                return self && self->store(obj, key, context);
            }
            static inline bool Unlink(iTJSDispatch2 *obj, const ttstr &key) {
                return obj &&
                    TJS_SUCCEEDED(obj->DeleteMember(0, key.c_str(), NULL, obj));
            }

            static iTJSDispatch2 *GetObject(const ttstr &name,
                                            iTJSDispatch2 *target = 0) {
                const tjs_char *p = name.c_str();
                const tjs_char *r = p;
                if(!target) {
                    target = TVPGetScriptDispatch();
                    if(!target)
                        return NULL;
                    target->Release(); // NoAddRef.
                }
                while(*p != 0) {
                    while(*r && *r != '.')
                        r++;
                    ttstr div(p, r - p);
                    if(!div.IsEmpty()) {
                        tTJSVariant val;
                        target = (TJS_SUCCEEDED(target->PropGet(
                                     0, div.c_str(), NULL, &val, target)))
                            ? val.AsObjectNoAddRef()
                            : 0;
                        if(!target)
                            return 0;
                    }
                    if(*r)
                        r++;
                    p = r;
                }
                return target;
            }
        };

        //--------------------------------------------------------------
        // TJS Function binder

        class FunctionInterface {
        public:
            virtual tjs_error invoke(tjs_uint32 flag, tTJSVariant *result,
                                     tjs_int numparams, tTJSVariant **param,
                                     iTJSDispatch2 *objthis) = 0;
            virtual ~FunctionInterface() {}
        };

        template <typename CB>
        class FunctionCallback : public FunctionInterface {
        public:
            typedef CB Callback;
            typedef MethodInvoker<Callback> Invoker;
            FunctionCallback(const Callback &cb) :
                _minparams(Invoker::Count), _callback(cb) {}
            tjs_error invoke(tjs_uint32 flag, tTJSVariant *result,
                             tjs_int numparams, tTJSVariant **param,
                             iTJSDispatch2 *objthis) {
                if(numparams < _minparams)
                    return TJS_E_BADPARAMCOUNT;
                return Invoker::Invoke(_callback, result, numparams, param,
                                       objthis);
            }

        private:
            tjs_int _minparams;
            const Callback _callback;
        };


        class FunctionStore : public StoreUtil {
        private:
            FunctionInterface *_function;

        protected:
            bool checkInstanceOf(const tjs_char *classname) {
                return compareString(classname, TJS_W("Function"));
            }

        public:
            template <typename T>
            FunctionStore(const T &func, bool autoStatic = false,
                          tjs_uint32 flag = StoreUtil::DEFAULT_FLAG) :
                StoreUtil(flag), _function(0) {
                if(autoStatic && MethodInvoker<T>::Static)
                    _flag |= TJS_STATICMEMBER;
                _function = static_cast<FunctionInterface *>(
                    new FunctionCallback<T>(func));
            }
            ~FunctionStore() {
                if(_function)
                    delete _function;
            }

            tjs_error TJS_INTF_METHOD
            FuncCall(tjs_uint32 flag, const tjs_char *membername,
                     tjs_uint32 *hint, tTJSVariant *result, tjs_int numparams,
                     tTJSVariant **param, iTJSDispatch2 *objthis) {
                if(membername)
                    return TJS_E_MEMBERNOTFOUND;
                return _function->invoke(flag, result, numparams, param,
                                         objthis);
            }

            template <typename T>
            static bool Link(iTJSDispatch2 *obj, const ttstr &key,
                             const T &func, iTJSDispatch2 *context = NULL) {
                return StoreUtil::Link(obj, key, new FunctionStore(func),
                                       context);
            }
        };

        //--------------------------------------------------------------
        // TJS Property binder

        class SetterInterface {
        public:
            virtual tjs_error invoke(tjs_uint32 flag, const tTJSVariant *param,
                                     iTJSDispatch2 *objthis) = 0;
            virtual ~SetterInterface() {}
        };
        class GetterInterface {
        public:
            virtual tjs_error invoke(tjs_uint32 flag, tTJSVariant *result,
                                     iTJSDispatch2 *objthis) = 0;
            virtual ~GetterInterface() {}
        };

        template <typename T>
        class SetterCallback;
        template <typename T>
        class GetterCallback;

#define SETTERARG                                                              \
    (tjs_uint32 flag, const tTJSVariant *param, iTJSDispatch2 *objthis)
#define GETTERARG                                                              \
    (tjs_uint32 flag, tTJSVariant * result, iTJSDispatch2 * objthis)

#define DEF_SPCB(TAG, IVG, ARGS, EXT)                                          \
    template <>                                                                \
    class TAG##Callback<tjs_error(*) ARGS> : public TAG##Interface {           \
        typedef tjs_error(*Callback) ARGS;                                     \
        const Callback _callback;                                              \
                                                                               \
    public:                                                                    \
        TAG##Callback(const Callback &cb) : _callback(cb) {} /**/              \
        tjs_error invoke IVG { return _callback EXT; }                         \
    }

        DEF_SPCB(Setter, SETTERARG, (const tTJSVariant *), (param));
        DEF_SPCB(Setter, SETTERARG, (iTJSDispatch2 *, const tTJSVariant *),
                 (objthis, param));

        DEF_SPCB(Getter, GETTERARG, (tTJSVariant *), (result));
        DEF_SPCB(Getter, GETTERARG, (iTJSDispatch2 *, tTJSVariant *),
                 (objthis, result));

#define DEF_SCCB(TAG, IVG, PRE1, ARGS, PRE2, EXT)                              \
    template <class C>                                                         \
    class TAG##Callback<tjs_error(PRE1 *) ARGS> : public TAG##Interface {      \
        typedef tjs_error(PRE1 *Callback) ARGS;                                \
        const Callback _callback;                                              \
                                                                               \
    public:                                                                    \
        TAG##Callback(const Callback &cb) : _callback(cb) {} /**/              \
        tjs_error invoke IVG {                                                 \
            C *self = 0;                                                       \
            return (objthis && MethodInvokerBase::GetSelf(objthis, self))      \
                ? ((PRE2 _callback)EXT)                                        \
                : TJS_E_NATIVECLASSCRASH;                                      \
        }                                                                      \
    }

        DEF_SCCB(Setter, SETTERARG, C::, (const tTJSVariant *), self->*,
                 (param));
        DEF_SCCB(Setter, SETTERARG, , (C *, const tTJSVariant *), ,
                 (self, param));

        DEF_SCCB(Getter, GETTERARG, C::, (tTJSVariant *) const, self->*,
                 (result));
        DEF_SCCB(Getter, GETTERARG, C::, (tTJSVariant *), self->*, (result));
        DEF_SCCB(Getter, GETTERARG, , (const C *, tTJSVariant *), ,
                 (self, result));
        DEF_SCCB(Getter, GETTERARG, , (C *, tTJSVariant *), , (self, result));

#undef DEF_SPCB
#undef DEF_SCCB
#undef SETTERARG
#undef GETTERARG

        class PropertyStore : public StoreUtil {
        private:
            SetterInterface *_setter;
            GetterInterface *_getter;

        protected:
            bool checkInstanceOf(const tjs_char *classname) {
                return compareString(classname, TJS_W("Property"));
            }

        public:
            PropertyStore() : _setter(0), _getter(0) {}
            template <typename SET, typename GET>
            PropertyStore(const SET &set, const GET &get) :
                _setter(0), _getter(0) {
                _setter = static_cast<SetterInterface *>(
                    new SetterCallback<SET>(set));
                _getter = static_cast<GetterInterface *>(
                    new GetterCallback<GET>(get));
            }
            template <typename SET>
            PropertyStore(const SET &set, int setter_only = 0) :
                _setter(0), _getter(0) {
                _setter = static_cast<SetterInterface *>(
                    new SetterCallback<SET>(set));
            }
            template <typename GET>
            PropertyStore(int getter_only, const GET &get) :
                _setter(0), _getter(0) {
                _getter = static_cast<GetterInterface *>(
                    new GetterCallback<GET>(get));
            }
            ~PropertyStore() {
                if(_setter)
                    delete _setter;
                if(_getter)
                    delete _getter;
            }

            tjs_error TJS_INTF_METHOD PropSet(tjs_uint32 flag,
                                              const tjs_char *membername,
                                              tjs_uint32 *hint,
                                              const tTJSVariant *param,
                                              iTJSDispatch2 *objthis) {
                if(membername)
                    return TJS_E_MEMBERNOTFOUND;
                return _setter ? _setter->invoke(flag, param, objthis)
                               : TJS_E_ACCESSDENYED;
            }

            tjs_error TJS_INTF_METHOD PropGet(tjs_uint32 flag,
                                              const tjs_char *membername,
                                              tjs_uint32 *hint,
                                              tTJSVariant *result,
                                              iTJSDispatch2 *objthis) {
                if(membername)
                    return TJS_E_MEMBERNOTFOUND;
                return _getter ? _getter->invoke(flag, result, objthis)
                               : TJS_E_ACCESSDENYED;
            }

            template <typename SET, typename GET>
            static bool Link(iTJSDispatch2 *obj, const ttstr &key,
                             const SET &set, const GET &get,
                             iTJSDispatch2 *context = NULL) {
                return StoreUtil::Link(obj, key, new PropertyStore(set, get),
                                       context);
            }
        };

        //--------------------------------------------------------------
        // TJS Constant binder

        class ConstantStore : public StoreUtil {
        private:
            tTJSVariant _value;

        protected:
            bool checkInstanceOf(const tjs_char *classname) {
                return (compareString(classname, TJS_W("Property")) ||
                        compareString(classname, TJS_W("Constant")));
            }

        public:
            template <typename T>
            ConstantStore(const T &value) : _value(value) {}
            ~ConstantStore() {}

            tjs_error TJS_INTF_METHOD PropSet(tjs_uint32 flag,
                                              const tjs_char *membername,
                                              tjs_uint32 *hint,
                                              const tTJSVariant *param,
                                              iTJSDispatch2 *objthis) {
                return membername ? TJS_E_MEMBERNOTFOUND : TJS_E_ACCESSDENYED;
            }

            tjs_error TJS_INTF_METHOD PropGet(tjs_uint32 flag,
                                              const tjs_char *membername,
                                              tjs_uint32 *hint,
                                              tTJSVariant *result,
                                              iTJSDispatch2 *objthis) {
                if(membername)
                    return TJS_E_MEMBERNOTFOUND;
                if(result)
                    *result = _value;
                return TJS_S_OK;
            }

            template <typename T>
            static bool Link(iTJSDispatch2 *obj, const ttstr &key,
                             const T &value) {
                return StoreUtil::Link(obj, key, new ConstantStore(value));
            }
        };

        //--------------------------------------------------------------
        // TJS Class binder

        class ClassEntryInterface {
        public:
            virtual ~ClassEntryInterface() {}
            virtual void release() { delete this; }
            virtual tjs_error createNew(void **instance, tjs_int numparams,
                                        tTJSVariant **param,
                                        iTJSDispatch2 *objthis) = 0;
            virtual tjs_error invalidate(void *instance) = 0;
            virtual tjs_error entryMembers(iTJSDispatch2 *classobj) = 0;
        };

        template <class C>
        struct AutoDelete {
            static void Delete(C *p) {
                if(p)
                    delete p;
            }
        };

        template <typename CTOR>
        class ClassEntryCallback : public ClassEntryInterface {
        public:
            typedef CTOR Callback;
            typedef FactoryInvoker<Callback> Invoker;
            typedef typename Invoker::Class Class;
            typedef void (*DeleteCB)(Class *);

            ClassEntryCallback(const Callback &cb, iTJSDispatch2 **clsobj,
                               DeleteCB del = &AutoDelete<Class>::Delete) :
                _minparams(Invoker::Count), _callback(cb), _delete(del),
                _outclsobj(clsobj) {}

            tjs_error createNew(void **instance, tjs_int numparams,
                                tTJSVariant **param, iTJSDispatch2 *objthis) {
                if(numparams < _minparams)
                    return TJS_E_BADPARAMCOUNT;
                Class *self = 0;
                tjs_error r =
                    Invoker::Invoke(_callback, self, numparams, param, objthis);
                if(instance)
                    *instance = reinterpret_cast<void *>(self);
                return r;
            }

            tjs_error invalidate(void *instance) {
                if(instance && _delete)
                    _delete(reinterpret_cast<Class *>(instance));
                return TJS_S_OK;
            }

            tjs_error entryMembers(iTJSDispatch2 *classobj) {
                if(_outclsobj)
                    *_outclsobj = classobj;
                return TJS_S_OK;
            }

        private:
            tjs_int _minparams;
            const Callback _callback;
            const DeleteCB _delete;
            iTJSDispatch2 **_outclsobj;
        };


        template <class C>
        class ClassStore;

        template <class C>
        class InstanceWrapper : public tTJSNativeInstance {
            C *_instance;

        public:
            typedef InstanceWrapper Self;
            typedef C Class;
            typedef ClassStore<Class> Store;
            typedef ClassEntryInterface *Entry;

            InstanceWrapper() : _instance(0) {}

            // iTJSNativeInstance::Construct
            tjs_error TJS_INTF_METHOD Construct(tjs_int numparams,
                                                tTJSVariant **param,
                                                iTJSDispatch2 *objthis) {
                Entry entry = GetEntry();
                return entry
                    ? entry->createNew(reinterpret_cast<void **>(&_instance),
                                       numparams, param, objthis)
                    : TJS_E_FAIL;
            }
            // iTJSNativeInstance::Invalidate
            void TJS_INTF_METHOD Invalidate() {
                if(_instance) {
                    Entry entry = GetEntry();
                    if(entry)
                        entry->invalidate(_instance);
                    _instance = 0;
                }
            }

            Class *getInstance() const { return _instance; }

        private:
            // NativeClassConstructor callback
            static tjs_error TJS_INTF_METHOD
            TJSConstruct(tTJSVariant *result, tjs_int numparams,
                         tTJSVariant **param, iTJSDispatch2 *objthis) {
                Self *self = GetSelf(objthis);
                if(!self)
                    return TJS_E_NATIVECLASSCRASH;
                return self->Construct(numparams, param, objthis);
            }
            // NativeClassMethod callback
            static tjs_error TJS_INTF_METHOD
            TJSFinalize(tTJSVariant *result, tjs_int numparams,
                        tTJSVariant **param, iTJSDispatch2 *objthis) {
                return TJS_S_OK;
            }

        protected:
            static Entry GetEntry() { return Store::GetClassEntry(); }
            static Self *GetSelf(iTJSDispatch2 *objthis) {
                return Store::GetInstanceWrapper(objthis);
            }

        public:
            static Class *GetInstance(iTJSDispatch2 *objthis) {
                Self *self = GetSelf(objthis);
                return self ? self->getInstance() : 0;
            }

            static void SetupClassObject(const tjs_char *classname,
                                         tTJSNativeClassForPlugin *classobj) {
                TJSNativeClassRegisterNCM(
                    classobj, TJS_W("finalize"),
                    TJSCreateNativeClassMethod(&TJSFinalize), classname,
                    nitMethod);
                TJSNativeClassRegisterNCM(
                    classobj, classname,
                    TJSCreateNativeClassConstructor(&TJSConstruct), classname,
                    nitMethod);
            }
        };

        template <class C>
        class ClassStore : public StoreUtil {
            tjs_int32 _id;
            ClassEntryInterface *_entry;
            static ClassStore *_Singleton;

        public:
            typedef InstanceWrapper<C> Wrapper;

            ClassStore(tjs_int32 id, ClassEntryInterface *entry) :
                StoreUtil(StoreUtil::DEFAULT_FLAG | TJS_HIDDENMEMBER |
                          TJS_STATICMEMBER),
                _id(id), _entry(entry) {}
            ~ClassStore() {
                if(_entry)
                    _entry->release();
                _entry = NULL;
                _Singleton = NULL;
            }

            tjs_int32 getID() const { return _id; }
            ClassEntryInterface *getEntry() const { return _entry; }

            static bool Link(iTJSDispatch2 *obj, const ttstr &key,
                             ClassEntryInterface *entry,
                             tjs_uint32 flag = StoreUtil::DEFAULT_FLAG) {
                iTJSDispatch2 *classobj = CreateClassObject(key, entry);
                if(!classobj)
                    return false;
                tTJSVariant val(classobj);
                classobj->Release();
                return obj &&
                    TJS_SUCCEEDED(
                           obj->PropSet(flag, key.c_str(), NULL, &val, obj));
            }
            template <typename CTOR>
            static bool Link(iTJSDispatch2 *obj, const ttstr &name,
                             const CTOR &ctor, iTJSDispatch2 **clsobj = 0,
                             tjs_uint32 flag = StoreUtil::DEFAULT_FLAG) {
                return Link(obj, name,
                            static_cast<ClassEntryInterface *>(
                                new ClassEntryCallback<CTOR>(ctor, clsobj)),
                            flag);
            }
            template <typename CTOR, typename DTOR>
            static bool Link(iTJSDispatch2 *obj, const ttstr &name,
                             const CTOR &ctor, const DTOR &dtor,
                             iTJSDispatch2 **clsobj = 0,
                             tjs_uint32 flag = StoreUtil::DEFAULT_FLAG) {
                return Link(
                    obj, name,
                    static_cast<ClassEntryInterface *>(
                        new ClassEntryCallback<CTOR>(ctor, clsobj, dtor)),
                    flag);
            }

        protected:
            // NativeClassFactory callback
            static iTJSNativeInstance *CreateWrapper() {
                return static_cast<iTJSNativeInstance *>(new Wrapper());
            }

            static iTJSDispatch2 *
            CreateClassObject(const ttstr &name, ClassEntryInterface *entry) {
                if(_Singleton) {
                    Error(ttstr(TJS_W("Class already created: ")) + name);
                    return 0;
                }

                const tjs_char *classname = name.c_str();

                tTJSNativeClassForPlugin *classobj =
                    TJSCreateNativeClassForPlugin(classname, &CreateWrapper);
                if(!classobj) {
                    Error(
                        ttstr(TJS_W("TJSCreateNativeClassForPlugin failed: ")) +
                        name);
                    return 0;
                }

                tjs_int32 id = TJSRegisterNativeClass(classname);
                TJSNativeClassSetClassID(classobj, id);

                _Singleton = new ClassStore(id, entry);
                if(!StoreUtil::Link(classobj, TJS_W("_*ClassStore*_"),
                                    static_cast<StoreUtil *>(_Singleton))) {
                    Error(ttstr(TJS_W("Class entry link failed: ")) + name);
                    classobj->Release();
                    return NULL;
                }

                Wrapper::SetupClassObject(classname, classobj);
                entry->entryMembers(classobj);

                return static_cast<iTJSDispatch2 *>(classobj);
            }

        public:
            static ClassEntryInterface *GetClassEntry() {
                return _Singleton ? _Singleton->getEntry() : 0;
            }
            static Wrapper *GetInstanceWrapper(iTJSDispatch2 *objthis) {
                if(!_Singleton)
                    return 0;
                Wrapper *wrapper = 0;
                return objthis &&
                        TJS_SUCCEEDED(objthis->NativeInstanceSupport(
                            TJS_NIS_GETINSTANCE, _Singleton->getID(),
                            reinterpret_cast<iTJSNativeInstance **>(&wrapper)))
                    ? wrapper
                    : 0;
            }
        };
        template <class C>
        ClassStore<C> *ClassStore<C>::_Singleton = 0;

        template <class C>
        struct ClassInstanceResolver {
            typedef InstanceWrapper<C> Wrapper;
        };
        template <class C>
        struct ClassInstanceResolver<const C> {
            typedef InstanceWrapper<C> Wrapper;
        };


        //--------------------------------------------------------------
        // TJS bind interface

        class BindUtil {
            bool _link, _error;
            iTJSDispatch2 *_store, *_context;

        public:
            BindUtil(bool link) :
                _link(link), _error(false), _store(0), _context(0) {
                _store = TVPGetScriptDispatch();
                if(_store)
                    _store->Release(); // NoAddRef.
                else
                    _error = true;
            }
            BindUtil(const ttstr &base, bool link) :
                _link(link), _error(false), _store(0), _context(0) {
                _store = StoreUtil::GetObject(base);
            }
            BindUtil(const ttstr &base, iTJSDispatch2 *root, bool link) :
                _link(link), _error(false), _store(0), _context(0) {
                _store = StoreUtil::GetObject(base, root);
            }
            BindUtil(iTJSDispatch2 *store, bool link) :
                _link(link), _error(false), _store(store), _context(0) {}

            virtual ~BindUtil() {}

            template <typename FUNC>
            BindUtil &Function(const ttstr &key, const FUNC &func) {
                if(_store)
                    _error |= !(
                        _link ? FunctionStore::Link(_store, key, func, _context)
                              /**/
                              : StoreUtil::Unlink(_store, key));
                return *this;
            }
            /**
             * プロパティを登録
             * @param key 登録プロパティ名
             * @param get 登録対象のgetter型の関数
             * @param set 登録対象のsetter型の関数
             * ※setter/getterどちらかが無い場合は 0 を指定します
             */
            template <typename GET, typename SET>
            BindUtil &Property(const ttstr &key, const GET &get,
                               const SET &set) {
                if(_store)
                    _error |= !(_link ? PropertyStore::Link(_store, key, set,
                                                            get, _context)
                                      /**/
                                      : StoreUtil::Unlink(_store, key));
                return *this;
            }
            /**
             * 定数を登録
             * @param key 登録定数名
             * @param value 登録対象の値(tTJSVariantのコンストラクタに渡せる型)
             * ※valueを返すgetterのみのプロパティを登録します
             */
            template <typename T>
            BindUtil &Constant(const ttstr &key, const T &value) {
                if(_store)
                    _error |= !(_link ? ConstantStore::Link(_store, key, value)
                                      /**/
                                      : StoreUtil::Unlink(_store, key));
                return *this;
            }
            /**
             * tTJSVariantを登録
             * @param key 登録名
             * @param value 登録対象の値(tTJSVariantのコンストラクタに渡せる型)
             * @param flag PropSetに渡すフラグ値
             */
            template <typename T>
            BindUtil &Variant(const ttstr &key, const T &value,
                              tjs_uint32 flag = StoreUtil::DEFAULT_FLAG) {
                if(_store) {
                    if(_link) {
                        tTJSVariant v(value);
                        _error |= TJS_FAILED(_store->PropSet(flag, key.c_str(),
                                                             NULL, &v, _store));
                    } else {
                        _error |= !StoreUtil::Unlink(_store, key);
                    }
                }
                return *this;
            }
            /**
             * クラスをを登録
             * @param key 登録クラス名
             * @param ctor 登録対象のfactory型の関数
             */
            template <typename CTOR>
            BindUtil &Class(const ttstr &key, const CTOR &ctor) {
                if(_store) {
                    typedef typename FactoryInvoker<CTOR>::Class TargetClass;
                    iTJSDispatch2 *rebase = 0;
                    _error |= !(_link ? ClassStore<TargetClass>::Link(
                                            _store, key, ctor, &rebase)
                                      /**/
                                      : StoreUtil::Unlink(_store, key));
                    _store = rebase;
                }
                return *this;
            }
            /**
             * クラスをを登録
             * @param key 登録クラス名
             * @param ctor 登録対象のfactory型の関数
             * @param dtor 登録対象のデストラクタ
             */
            template <typename CTOR, typename DTOR>
            BindUtil &Class(const ttstr &key, const CTOR &ctor,
                            const DTOR &dtor) {
                if(_store) {
                    typedef typename FactoryInvoker<CTOR>::Class TargetClass;
                    iTJSDispatch2 *rebase = 0;
                    _error |= !(_link ? ClassStore<TargetClass>::Link(
                                            _store, key, ctor, dtor, &rebase)
                                      /**/
                                      : StoreUtil::Unlink(_store, key));
                    _store = rebase;
                }
                return *this;
            }

            /**
             * 以降のFunction/Property登録コンテキストを変更
             * @param context コンテキスト
             */
            BindUtil &SetContext(iTJSDispatch2 *context = NULL) {
                _context = context;
                return *this;
            }

            /**
             * エラーチェック
             * 今までの登録／解除動作でエラーがなかったかどうか確認
             * @return valid エラーが無い場合はtrue
             */
            bool IsValid() const { return !_error; }


            /**
             * iTJSDispatch2から任意のクラスインスタンスの変換
             * @param obj 変換対象オブジェクト
             * @param instance 変換したいクラスの型ポインタ（タグ用ダミー）
             * @return ポインタ
             */
            template <class C>
            static C *GetInstance(iTJSDispatch2 *obj, C *instance) {
                return MethodInvokerBase::GetSelf(obj, instance) ? instance : 0;
            }

            /**
             * 任意階層にあるオブジェクトを取得
             * @param base 基点階層（"."で区切って階層指定可能）
             * @param store 基点オブジェクト（省略時orNULLの場合はglobal）
             * @return オブジェクトがあればiTJSDispatch2ポインタ，なければNULL
             */
            static inline iTJSDispatch2 *GetObject(const ttstr &base,
                                                   iTJSDispatch2 *root = 0) {
                return StoreUtil::GetObject(base, root);
            }
        };

    } // namespace Detail


    // Public Entries

    typedef Detail::BindUtil BindUtil;

    using Detail::ExceptionRethrowTrap;


} // namespace SimpleBinder


// util macro (for BindUtil::SetContext)
#define SIMPLEBINDER_CUSTOM_CONTEXT_RESOLV(CLASS)                              \
    template <>                                                                \
    struct SimpleBinder::Detail::ClassInstanceResolver<CLASS> {                \
        typedef CLASS Wrapper;                                                 \
    };                                                                         \
    template <>                                                                \
    struct SimpleBinder::Detail::ClassInstanceResolver<const CLASS> {          \
        typedef CLASS Wrapper;                                                 \
    }
// -> implement "CLASS* CLASS::GetInstance(iTJSDispatch2*)" static method.
//    and this macro use before BindUtil using.
