
#ifndef tjsObjectExtendableH
#define tjsObjectExtendableH

#include "tjsObject.h"

namespace TJS {
    // ½dp³ÍT|[gµÈ¢
    class tTJSExtendableObject : public tTJSCustomObject {
        typedef tTJSCustomObject inherited;

    protected:
        iTJSDispatch2 *SuperClass;

        /**
         * @param global :
         * ùèNXðõ·éIuWFNg(ÊÍO[o)
         * @param classname : ùèNX¼
         */
        void ExtendsClass(iTJSDispatch2 *global, const ttstr &classname);

    public:
        tTJSExtendableObject() : SuperClass(nullptr) {}

        virtual ~tTJSExtendableObject();

        iTJSDispatch2 *GetSuper() { return SuperClass; }

        const iTJSDispatch2 *GetSuper() const { return SuperClass; }

        void SetSuper(iTJSDispatch2 *dsp);

        tjs_error FuncCall(tjs_uint32 flag, const tjs_char *membername,
                           tjs_uint32 *hint, tTJSVariant *result,
                           tjs_int numparams, tTJSVariant **param,
                           iTJSDispatch2 *objthis);

        tjs_error CreateNew(tjs_uint32 flag, const tjs_char *membername,
                            tjs_uint32 *hint, iTJSDispatch2 **result,
                            tjs_int numparams, tTJSVariant **param,
                            iTJSDispatch2 *objthis);

        tjs_error PropGet(tjs_uint32 flag, const tjs_char *membername,
                          tjs_uint32 *hint, tTJSVariant *result,
                          iTJSDispatch2 *objthis);

        tjs_error PropSet(tjs_uint32 flag, const tjs_char *membername,
                          tjs_uint32 *hint, const tTJSVariant *param,
                          iTJSDispatch2 *objthis);

        tjs_error IsInstanceOf(tjs_uint32 flag, const tjs_char *membername,
                               tjs_uint32 *hint, const tjs_char *classname,
                               iTJSDispatch2 *objthis);

        tjs_error GetCount(tjs_int *result, const tjs_char *membername,
                           tjs_uint32 *hint, iTJSDispatch2 *objthis);

        tjs_error DeleteMember(tjs_uint32 flag, const tjs_char *membername,
                               tjs_uint32 *hint, iTJSDispatch2 *objthis);

        tjs_error Invalidate(tjs_uint32 flag, const tjs_char *membername,
                             tjs_uint32 *hint, iTJSDispatch2 *objthis);

        tjs_error IsValid(tjs_uint32 flag, const tjs_char *membername,
                          tjs_uint32 *hint, iTJSDispatch2 *objthis);

        tjs_error Operation(tjs_uint32 flag, const tjs_char *membername,
                            tjs_uint32 *hint, tTJSVariant *result,
                            const tTJSVariant *param, iTJSDispatch2 *objthis);

        tjs_error NativeInstanceSupport(tjs_uint32 flag, tjs_int32 classid,
                                        iTJSNativeInstance **pointer);

        // X[p[NXÌo^Ææ¾ðT|[g
        tjs_error ClassInstanceInfo(tjs_uint32 flag, tjs_uint num,
                                    tTJSVariant *value);
    };
} // namespace TJS
#endif // tjsObjectExtendableH
