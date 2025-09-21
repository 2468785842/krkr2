//
// Created by LiDon on 2025/9/11.
//

#include <spdlog/spdlog.h>

#include "PSBMedia.h"

#include "UtilStreams.h"

namespace PSB {
#define LOGGER spdlog::get("plugin")

    void PSBMedia::NormalizeDomainName(ttstr &name) {
        tjs_int dotIndex = name.IndexOf(TJS_W('.'));
        if(dotIndex == -1)
            return;
        name = name.SubString(0, dotIndex) +
            name.SubString(dotIndex, name.GetLen()).AsLowerCase();
    }

    void PSBMedia::NormalizePathName(ttstr &name) {
        // province(_p), mask(_m)
    }

    bool PSBMedia::CheckExistentStorage(const ttstr &name) {
        return _resources.find(name.AsStdString()) != _resources.end();
    }

    tTJSBinaryStream *PSBMedia::Open(const ttstr &name, tjs_uint32 flags) {
        auto memoryStream = new tTVPMemoryStream();
        auto res = _resources[name.AsStdString()];
        memoryStream->WriteBuffer(res.data.data(), res.data.size());
        memoryStream->Seek(0, TJS_BS_SEEK_SET);
        return memoryStream;
    }

    void PSBMedia::GetListAt(const ttstr &name, iTVPStorageLister *lister) {
        LOGGER->error("TODO: PSBMedia GetListAt");
    }

    void PSBMedia::GetLocallyAccessibleName(ttstr &name) {
        LOGGER->error("can't get GetLocallyAccessibleName from {}!",
                      name.AsStdString());
    }

    void PSBMedia::add(const std::string &name,
                       const std::shared_ptr<PSBResource> &resource) {
        this->_resources[name] = *resource;
    }
} // namespace PSB