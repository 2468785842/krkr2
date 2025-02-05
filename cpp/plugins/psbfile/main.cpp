//
// Created by lidong on 2025/1/31.
// TODO: implement psbfile.dll plugin
//
#include <zlib.h>
#include "ncbind.hpp"
#include "PSBHeader.h"

#define NCB_MODULE_NAME TJS_W("psbfile.dll")

static std::shared_ptr<spdlog::logger> logger{};

void initPsbFile() {
    logger = spdlog::get("plugin");
    logger->info("initPsbFile");
}

void deInitPsbFile() {
    logger->info("deInitPsbFile");
    logger = nullptr;
}

class PSBFile {
public:
    tTJSDispatch obj{};
    PSBFile() { logger->info("PSBFile::constructor"); }

    static tjs_error getRoot(tTJSVariant *r, tjs_int n, tTJSVariant **p,
                             iTJSDispatch2 *obj) {
        // TODO:
        logger->info("PSBFile::getRoot");
        logger->critical("PSBFile::getRoot not implement");
        return TJS_S_OK;
    }

    static tjs_error setRoot(tTJSVariant *r, tjs_int n, tTJSVariant **p,
                             iTJSDispatch2 *obj) {
        // TODO:
        logger->info("PSBFile::setRoot");
        logger->critical("PSBFile::setRoot not implement");
        return TJS_S_OK;
    }

    static tjs_error load(tTJSVariant *r, tjs_int n, tTJSVariant **p,
                          iTJSDispatch2 *obj) {
        logger->info("PSBFile::load");
        if(r->Type() == tvtString) {
            ttstr path{ r };
            if(!openPSBFile(path)) {
                logger->info("cannot open psb file : {}", path.AsStdString());
            }
        }
        if(r->Type() == tvtOctet) {
            logger->info("PSBFile::load stream");
        }
        logger->info("PSBFile::load invoke over");
        auto *self = ncbInstanceAdaptor<PSBFile>::GetNativeInstance(obj);
        *r = &self->obj;
        return TJS_S_OK;
    }

    /**
     * file type: *.PIMG
     * @param filePath
     */
    static bool openPSBFile(const ttstr &filePath) {
        logger->info("PSBFile::load path: {}", filePath.AsStdString());
        std::unique_ptr<tTJSBinaryStream> stream{ TVPCreateStream(filePath) };
        if(!stream) {
            return false;
        }
        size_t readSize = stream->GetSize();
        if(readSize < 9) {
            return false;
        }

        auto *buffer = new int[readSize];
        stream->ReadBuffer(buffer, readSize);

        if(10 < readSize && buffer[0] == /* fdm */ 0x66646d) {
            auto originalLen = readSize - 8;
            uLong compressedLen = compressBound(originalLen);
            auto *compressed = new u_char[compressedLen];
            int code = uncompress(compressed, &compressedLen,
                                  reinterpret_cast<const Bytef *>(buffer[2]),
                                  originalLen);
            if(code == 0) {
                delete[] buffer;
                return false;
            }
        }

        PSB::PSBHeader psbHeader{};
        PSB::parsePSBHeader(buffer, &psbHeader);
        if(std::memcmp(buffer, &PSB::PsbSignature, /* signature len */ 4) !=
           0) {
            return false;
        }

        if(psbHeader.version > 3) {
            logger->critical("not support psb file format version > 3");
        }

        delete[] buffer;
        return true;
    }
};
NCB_REGISTER_CLASS(PSBFile) {
    NCB_CONSTRUCTOR(());
    RawCallback(TJS_W("root"), &ClassT::getRoot, &ClassT::setRoot, 0);
    //    Method(TJS_W("load"), &ClassT::load);
    RawCallback(TJS_W("load"), &ClassT::load, 0);
};

NCB_PRE_REGIST_CALLBACK(initPsbFile);
NCB_POST_UNREGIST_CALLBACK(deInitPsbFile);
