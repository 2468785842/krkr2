//
// Created by lidong on 2025/1/31.
// TODO: implement psbfile.dll plugin
// reference https://github.com/UlyssesWu/FreeMote
//
#include <zlib.h>
#include <spdlog/spdlog.h>
#include <cassert>

#include "tjs.h"
#include "ncbind.hpp"
#include "PSBFile.h"
#include "PSBHeader.h"
#include "PSBValue.h"

#define NCB_MODULE_NAME TJS_W("psbfile.dll")

#define LOGGER spdlog::get("plugin")

void initPsbFile() { LOGGER->info("initPsbFile"); }

void deInitPsbFile() { LOGGER->info("deInitPsbFile"); }


static tjs_error getRoot(tTJSVariant *r, tjs_int n, tTJSVariant **p,
                         iTJSDispatch2 *obj) {
    // TODO:
    LOGGER->critical("PSBFile::getRoot not implement");
    auto *self = ncbInstanceAdaptor<PSB::PSBFile>::GetNativeInstance(obj);
    // *r = &self->root;
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
    bool loadSuccess = true;
    auto *self = ncbInstanceAdaptor<PSB::PSBFile>::GetNativeInstance(obj);
    if((*p)->Type() == tvtString) {
        ttstr path{ **p };
        if(!self->loadPSBFile(path)) {
            LOGGER->info("cannot load psb file : {}", path.AsStdString());
            loadSuccess = false;
        }
    }
    if((*p)->Type() == tvtOctet) {
        LOGGER->info("PSBFile::load stream");
        loadSuccess = false;
    }
    *r = loadSuccess;
    return TJS_S_OK;
}

using namespace PSB;
NCB_REGISTER_CLASS(PSBFile) {
    NCB_CONSTRUCTOR(());
    RawCallback(TJS_W("root"), &getRoot, &setRoot, 0);
    //    Method(TJS_W("load"), &ClassT::load);
    RawCallback(TJS_W("load"), &load, 0);
};

NCB_PRE_REGIST_CALLBACK(initPsbFile);
NCB_POST_UNREGIST_CALLBACK(deInitPsbFile);
