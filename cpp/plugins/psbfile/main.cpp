//
// Created by lidong on 2025/1/31.
// ref: https://github.com/number201724/psbfile
// ref: https://github.com/UlyssesWu/FreeMote
//
#include <cassert>

#include "tjs.h"
#include "ncbind.hpp"
#include "PSBFile.h"
#include "PSBHeader.h"
#include "PSBMedia.h"
#include "PSBMediaRegistry.h"
#include "PSBValue.h"
#include "UtilStreams.h"

#define NCB_MODULE_NAME TJS_W("psbfile.dll")

#include "log/TVPLog.h"

using namespace PSB;

void initPsbFile() { initPSBMedia(); }

void deInitPsbFile() { deInitPSBMedia(); }

static tjs_error getRoot(tTJSVariant *r, tjs_int n, tTJSVariant **p,
                         iTJSDispatch2 *obj) {
    auto *self = ncbInstanceAdaptor<PSB::PSBFile>::GetNativeInstance(obj);
    if(!self)
        return TJS_E_NATIVECLASSCRASH;
    if(!r)
        return TJS_S_OK;
    const auto objects = self->getObjects();
    *r = objects ? objects->toTJSVal() : PSBDictionary{}.toTJSVal();
    return TJS_S_OK;
}

static tjs_error load(tTJSVariant *r, tjs_int count, tTJSVariant **p,
                      iTJSDispatch2 *obj) {
    bool loadSuccess = true;
    auto *self = ncbInstanceAdaptor<PSB::PSBFile>::GetNativeInstance(obj);
    if(!self)
        return TJS_E_NATIVECLASSCRASH;
    if(count != 1) {
        return TJS_E_BADPARAMCOUNT;
    }

    if((*p)->Type() == tvtString) {
        ttstr path{ **p };
        if(!self->loadPSBFile(path)) {
            G_PluginLog.info("cannot load psb file : {}", path.AsStdString());
            loadSuccess = false;
        }
        if(loadSuccess)
            registerRootResources(path, *self);
    } else if((*p)->Type() == tvtOctet) {
        const auto *octet = (*p)->AsOctetNoAddRef();
        if(!octet) {
            loadSuccess = false;
        } else {
            tTVPMemoryStream stream(octet->GetData(), octet->GetLength());
            loadSuccess = self->loadPSBStream(&stream);
        }
    } else {
        return TJS_E_INVALIDPARAM;
    }

    if(r != nullptr)
        *r = tTJSVariant(loadSuccess);
    return TJS_S_OK;
}

template <typename T>
class PSBFileConvertor {
    typedef ncbTypeConvertor::Stripper<PSBFile>::Type ClassT;
    typedef ncbInstanceAdaptor<ClassT> AdaptorT;

public:
    PSBFileConvertor() = default;
    virtual ~PSBFileConvertor() = default;

    virtual void operator()(T *&dst, const tTJSVariant &src) {
        if(src.Type() == tvtObject) {
            dst = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef());
        }
    }

    void operator()(tTJSVariant &dst, const T *&src) {
        if(src != nullptr) {
            if(iTJSDispatch2 *adpObj = AdaptorT::CreateAdaptor(src)) {
                dst = tTJSVariant(adpObj, adpObj);
                adpObj->Release();
            }
        } else {
            dst.Clear();
        }
    }
};

NCB_SET_CONVERTOR(PSBFile, PSBFileConvertor<PSBFile>);
NCB_SET_CONVERTOR(const PSBFile *, PSBFileConvertor<const PSBFile>);

static tjs_error PSBFileFactory(PSBFile **result, tjs_int count,
                                tTJSVariant **params, iTJSDispatch2 *_) {
    auto psbFile = std::make_unique<PSBFile>();
    if(count > 1)
        return TJS_E_BADPARAMCOUNT;
    if(count == 1 && (*params)->Type() == tvtString) {
        ttstr path{ *params[0] };
        if(psbFile->loadPSBFile(path))
            registerRootResources(path, *psbFile);
    } else if(count == 1 && (*params)->Type() == tvtOctet) {
        const auto *octet = (*params)->AsOctetNoAddRef();
        if(octet) {
            tTVPMemoryStream stream(octet->GetData(), octet->GetLength());
            psbFile->loadPSBStream(&stream);
        }
    } else if(count != 0) {
        return TJS_E_INVALIDPARAM;
    }
    *result = psbFile.release();
    return TJS_S_OK;
}

NCB_REGISTER_CLASS(PSBFile) {
    Factory(PSBFileFactory);
    RawCallback(TJS_W("root"), &getRoot, 0, 0);
    RawCallback(TJS_W("load"), &load, 0);
}

NCB_PRE_REGIST_CALLBACK(initPsbFile);
NCB_POST_UNREGIST_CALLBACK(deInitPsbFile);
