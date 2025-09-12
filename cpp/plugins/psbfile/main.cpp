//
// Created by lidong on 2025/1/31.
// TODO: implement psbfile.dll plugin
// reference https://github.com/UlyssesWu/FreeMote
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
static PSBMedia* psbMedia = nullptr;

void initPsbFile() {
    psbMedia = new PSBMedia();
    TVPRegisterStorageMedia(psbMedia);
    psbMedia->Release();
    LOGGER->info("initPsbFile");
}

void deInitPsbFile() {
    if (psbMedia != nullptr) {
        TVPUnregisterStorageMedia(psbMedia);
    }
    LOGGER->info("deInitPsbFile");
}


static tjs_error getRoot(tTJSVariant *r, tjs_int n, tTJSVariant **p,
                         iTJSDispatch2 *obj) {
    auto *self = ncbInstanceAdaptor<PSB::PSBFile>::GetNativeInstance(obj);
    iTJSDispatch2 *dic = TJSCreateCustomObject();
    auto objs = self->getObjects();
    for(const auto &[k, v] : *objs) {
        tTJSVariant tmp = v->toTJSVal();
        dic->PropSet(TJS_MEMBERENSURE, ttstr{ k }.c_str(), nullptr, &tmp, dic);
    }
    *r = tTJSVariant{ dic, dic };
    dic->Release();
    return TJS_S_OK;
}

static tjs_error setRoot(tTJSVariant *r, tjs_int n, tTJSVariant **p,
                         iTJSDispatch2 *obj) {
    // TODO:
    LOGGER->critical("PSBFile::setRoot not implement");
    return TJS_S_OK;
}

static tjs_error load(tTJSVariant *r, tjs_int n, tTJSVariant **p,
                      iTJSDispatch2 *obj) {
    auto *self = ncbInstanceAdaptor<PSB::PSBFile>::GetNativeInstance(obj);
    if((*p)->Type() == tvtString) {
        ttstr path{ **p };
        if(!self->loadPSBFile(path)) {
            LOGGER->info("cannot load psb file : {}", path.AsStdString());
        }
        auto objs = self->getObjects();
        for (const auto &[k, v] : *objs) {
            const auto &res = std::dynamic_pointer_cast<PSBResource>(v);
            if (res == nullptr) continue;
            ttstr pathN{k};
            psbMedia->NormalizeDomainName(path);
            psbMedia->NormalizePathName(pathN);
            psbMedia->add((path + TJS_W("/") + pathN).AsStdString(), res);
        }
    }
    if((*p)->Type() == tvtOctet) {
        LOGGER->info("PSBFile::load stream");
    }
    return TJS_S_OK;
}

NCB_REGISTER_CLASS(PSBFile) {
    NCB_CONSTRUCTOR(());
    RawCallback(TJS_W("root"), &getRoot, &setRoot, 0);
    RawCallback(TJS_W("load"), &load, 0);
};

NCB_PRE_REGIST_CALLBACK(initPsbFile);
NCB_POST_UNREGIST_CALLBACK(deInitPsbFile);
