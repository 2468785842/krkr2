//
// Created by lidong on 2025/1/31.
// TODO: implement psbfile.dll plugin
// ref: https://github.com/number201724/psbfile
// ref: https://github.com/UlyssesWu/FreeMote
//
#include <spdlog/spdlog.h>
#include <cassert>

#include "tjs.h"
#include "ncbind.hpp"
#include "PSBFile.h"
#include "PSBHeader.h"
#include "PSBMedia.h"
#include "PSBValue.h"

#define NCB_MODULE_NAME TJS_W("psbfile.dll")

#define LOGGER spdlog::get("plugin")

using namespace PSB;
static PSBMedia *psbMedia = nullptr;

void initPsbFile() {
    psbMedia = new PSBMedia();
    TVPRegisterStorageMedia(psbMedia);
    psbMedia->Release();
    LOGGER->info("initPsbFile");
}

void deInitPsbFile() {
    if(psbMedia != nullptr) {
        TVPUnregisterStorageMedia(psbMedia);
    }
    LOGGER->info("deInitPsbFile");
}

static tjs_error getRoot(tTJSVariant *r, tjs_int n, tTJSVariant **p,
                         iTJSDispatch2 *obj) {
    auto *self = ncbInstanceAdaptor<PSB::PSBFile>::GetNativeInstance(obj);
    iTJSDispatch2 *dic = TJSCreateCustomObject();
    auto objs = self->getObjects();
    if(objs != nullptr) {
        for(const auto &[k, v] : *objs) {
            tTJSVariant tmp = v->toTJSVal();
            dic->PropSet(TJS_MEMBERENSURE, ttstr{ k }.c_str(), nullptr, &tmp,
                         dic);
        }
    }
    *r = tTJSVariant{ dic, dic };
    dic->Release();
    return TJS_S_OK;
}

static tjs_error load(tTJSVariant *r, tjs_int count, tTJSVariant **p,
                      iTJSDispatch2 *obj) {
    bool loadSuccess = true;
    auto *self = ncbInstanceAdaptor<PSB::PSBFile>::GetNativeInstance(obj);
    if(count != 1) {
        return TJS_E_BADPARAMCOUNT;
    }

    if((*p)->Type() == tvtString) {
        ttstr path{ **p };
        if(!self->loadPSBFile(path)) {
            LOGGER->info("cannot load psb file : {}", path.AsStdString());
            loadSuccess = false;
        }
        auto objs = self->getObjects();
        for(const auto &[k, v] : *objs) {
            const auto &res = std::dynamic_pointer_cast<PSBResource>(v);
            if(res == nullptr)
                continue;
            ttstr pathN{ k };
            psbMedia->NormalizeDomainName(path);
            psbMedia->NormalizePathName(pathN);
            psbMedia->add((path + TJS_W("/") + pathN).AsStdString(), res);
        }
    } else if((*p)->Type() == tvtOctet) {
        LOGGER->critical("PSBFile::load stream no implement!");
        loadSuccess = false;
    } else {
        return TJS_E_INVALIDPARAM;
    }

    if(r != nullptr)
        *r = tTJSVariant(loadSuccess);
    return TJS_S_OK;
}

// 因为有两种版本的psbfile插件调用方式不一样
// TODO: 第一种（新) 实现有问题, 可能忽略了某些东西
// var psbfile = new PSBFile();
// psbfile.load("xxxx.PIMG");
// 第二种（旧)
// new PSBFile("xxxx.PIMG");

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
    PSBFile *psbFile = nullptr;
    if(count == 0) {
        psbFile = new PSBFile();
    } else if(count == 1 && (*params)->Type() == tvtString) {
        ttstr path{ *params[0] };
        psbFile = new PSBFile();
        psbFile->loadPSBFile(path);
    } else {
        return TJS_E_INVALIDPARAM;
    }
    *result = psbFile;
    return TJS_S_OK;
}

NCB_REGISTER_CLASS(PSBFile) {
    Factory(PSBFileFactory);
    RawCallback(TJS_W("root"), &getRoot, 0, 0);
    RawCallback(TJS_W("load"), &load, 0);
}

NCB_PRE_REGIST_CALLBACK(initPsbFile);
NCB_POST_UNREGIST_CALLBACK(deInitPsbFile);
