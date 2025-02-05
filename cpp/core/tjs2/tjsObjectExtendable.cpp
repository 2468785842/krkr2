//---------------------------------------------------------------------------
// TJS2 "ExtendableObject" class implementation
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjsObjectExtendable.h"

namespace TJS {

    tTJSExtendableObject::~tTJSExtendableObject() {
        if(SuperClass) {
            SuperClass->Release();
            SuperClass = nullptr;
        }
    }

    void tTJSExtendableObject::SetSuper(iTJSDispatch2 *dsp) {
        if(SuperClass) {
            SuperClass->Release();
            SuperClass = nullptr;
        }
        SuperClass = dsp;
        SuperClass->AddRef();
    }

    void tTJSExtendableObject::ExtendsClass(iTJSDispatch2 *global,
                                            const ttstr &classname) {
        tTJSVariant val;
        tjs_error er = global->PropGet(TJS_MEMBERMUSTEXIST, classname.c_str(),
                                       nullptr, &val, global);
        if(TJS_FAILED(er))
            TJSThrowFrom_tjs_error(er, classname.c_str());

        SetSuper(val.AsObjectNoAddRef());
    }

    tjs_error
    tTJSExtendableObject::FuncCall(tjs_uint32 flag, const tjs_char *membername,
                                   tjs_uint32 *hint, tTJSVariant *result,
                                   tjs_int numparams, tTJSVariant **param,
                                   iTJSDispatch2 *objthis) {
        tjs_error hr = inherited::FuncCall(flag, membername, hint, result,
                                           numparams, param, objthis);
        if(hr == TJS_E_MEMBERNOTFOUND && SuperClass != nullptr &&
           membername != nullptr) {
            hr = SuperClass->FuncCall(flag, membername, hint, result, numparams,
                                      param, objthis);
        }
        return hr;
    }

    tjs_error
    tTJSExtendableObject::CreateNew(tjs_uint32 flag, const tjs_char *membername,
                                    tjs_uint32 *hint, iTJSDispatch2 **result,
                                    tjs_int numparams, tTJSVariant **param,
                                    iTJSDispatch2 *objthis) {
        tjs_error hr = inherited::CreateNew(flag, membername, hint, result,
                                            numparams, param, objthis);
        if(hr == TJS_E_MEMBERNOTFOUND && SuperClass != nullptr &&
           membername != nullptr) {
            hr = SuperClass->CreateNew(flag, membername, hint, result,
                                       numparams, param, objthis);
        }
        return hr;
    }

    tjs_error tTJSExtendableObject::PropGet(tjs_uint32 flag,
                                            const tjs_char *membername,
                                            tjs_uint32 *hint,
                                            tTJSVariant *result,
                                            iTJSDispatch2 *objthis) {
        tjs_error hr =
            inherited::PropGet(flag, membername, hint, result, objthis);
        if(hr == TJS_E_MEMBERNOTFOUND && SuperClass != nullptr &&
           membername != nullptr) {
            hr = SuperClass->PropGet(flag, membername, hint, result, objthis);
        }
        return hr;
    }

    tjs_error tTJSExtendableObject::PropSet(tjs_uint32 flag,
                                            const tjs_char *membername,
                                            tjs_uint32 *hint,
                                            const tTJSVariant *param,
                                            iTJSDispatch2 *objthis) {
        tjs_error hr =
            inherited::PropSet(flag, membername, hint, param, objthis);
        if(hr == TJS_E_MEMBERNOTFOUND && SuperClass != nullptr &&
           membername != nullptr) {
            hr = SuperClass->PropSet(flag, membername, hint, param, objthis);
        }
        return hr;
    }

    tjs_error tTJSExtendableObject::IsInstanceOf(tjs_uint32 flag,
                                                 const tjs_char *membername,
                                                 tjs_uint32 *hint,
                                                 const tjs_char *classname,
                                                 iTJSDispatch2 *objthis) {
        tjs_error hr =
            inherited::IsInstanceOf(flag, membername, hint, classname, objthis);
        if(hr == TJS_E_MEMBERNOTFOUND && SuperClass != nullptr &&
           membername != nullptr) {
            hr = SuperClass->IsInstanceOf(flag, membername, hint, classname,
                                          objthis);
        }
        return hr;
    }

    tjs_error tTJSExtendableObject::GetCount(tjs_int *result,
                                             const tjs_char *membername,
                                             tjs_uint32 *hint,
                                             iTJSDispatch2 *objthis) {
        tjs_error hr = inherited::GetCount(result, membername, hint, objthis);
        if(hr == TJS_E_MEMBERNOTFOUND && SuperClass != nullptr &&
           membername != nullptr) {
            hr = SuperClass->GetCount(result, membername, hint, objthis);
        }
        return hr;
    }

    tjs_error tTJSExtendableObject::DeleteMember(tjs_uint32 flag,
                                                 const tjs_char *membername,
                                                 tjs_uint32 *hint,
                                                 iTJSDispatch2 *objthis) {
        tjs_error hr = inherited::DeleteMember(flag, membername, hint, objthis);
        if(hr == TJS_E_MEMBERNOTFOUND && SuperClass != nullptr &&
           membername != nullptr) {
            hr = SuperClass->DeleteMember(flag, membername, hint, objthis);
        }
        return hr;
    }

    tjs_error tTJSExtendableObject::Invalidate(tjs_uint32 flag,
                                               const tjs_char *membername,
                                               tjs_uint32 *hint,
                                               iTJSDispatch2 *objthis) {
        tjs_error hr = inherited::Invalidate(flag, membername, hint, objthis);
        if(hr == TJS_E_MEMBERNOTFOUND && SuperClass != nullptr &&
           membername != nullptr) {
            hr = SuperClass->Invalidate(flag, membername, hint, objthis);
        }
        if(membername == nullptr) {
            SuperClass->Invalidate(flag, membername, hint, objthis);
        }
        return hr;
    }

    tjs_error tTJSExtendableObject::IsValid(tjs_uint32 flag,
                                            const tjs_char *membername,
                                            tjs_uint32 *hint,
                                            iTJSDispatch2 *objthis) {
        tjs_error hr = inherited::IsValid(flag, membername, hint, objthis);
        if(hr == TJS_E_MEMBERNOTFOUND && SuperClass != nullptr &&
           membername != nullptr) {
            hr = SuperClass->IsValid(flag, membername, hint, objthis);
        }
        return hr;
    }

    tjs_error tTJSExtendableObject::Operation(
        tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
        tTJSVariant *result, const tTJSVariant *param, iTJSDispatch2 *objthis) {
        tjs_error hr = inherited::Operation(flag, membername, hint, result,
                                            param, objthis);
        if(hr == TJS_E_MEMBERNOTFOUND && SuperClass != nullptr &&
           membername != nullptr) {
            hr = SuperClass->Operation(flag, membername, hint, result, param,
                                       objthis);
        }
        return hr;
    }

    tjs_error tTJSExtendableObject::NativeInstanceSupport(
        tjs_uint32 flag, tjs_int32 classid, iTJSNativeInstance **pointer) {
        tjs_error hr = inherited::NativeInstanceSupport(flag, classid, pointer);
        if(hr != TJS_S_OK && SuperClass != nullptr &&
           flag == TJS_NIS_GETINSTANCE) {
            hr = SuperClass->NativeInstanceSupport(flag, classid, pointer);
        }
        return hr;
    }

    tjs_error tTJSExtendableObject::ClassInstanceInfo(tjs_uint32 flag,
                                                      tjs_uint num,
                                                      tTJSVariant *value) {
        if(flag == TJS_CII_SET_SUPRECLASS) {
            SetSuper(value->AsObjectNoAddRef());
            return TJS_S_OK;
        } else if(flag == TJS_CII_GET_SUPRECLASS) {
            if(SuperClass) {
                *value = tTJSVariant(SuperClass, SuperClass);
                return TJS_S_OK;
            } else {
                return TJS_E_FAIL;
            }
        } else {
            return inherited::ClassInstanceInfo(flag, num, value);
        }
    }

} // namespace TJS
