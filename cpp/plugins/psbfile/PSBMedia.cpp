//
// Created by LiDon on 2025/9/11.
//

#include "PSBMedia.h"

#include "UtilStreams.h"
#include "MsgIntf.h"

#include "log/TVPLog.h"

namespace PSB {
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
        const auto found = _resources.find(name.AsStdString());
        if((flags & TJS_BS_ACCESS_MASK) != TJS_BS_READ ||
           found == _resources.end())
            TVPThrowExceptionMessage(TVPCannotOpenStorage, name);
        auto memoryStream = std::make_unique<tTVPMemoryStream>();
        const auto &res = found->second;
        memoryStream->WriteBuffer(res.data.data(), res.data.size());
        memoryStream->Seek(0, TJS_BS_SEEK_SET);
        return memoryStream.release();
    }

    void PSBMedia::GetListAt(const ttstr &name, iTVPStorageLister *lister) {
        std::string prefix = name.AsStdString();
        if(!prefix.empty() && prefix.back() != '/')
            prefix += '/';
        for(const auto &[key, resource] : _resources) {
            if(key.compare(0, prefix.size(), prefix) != 0)
                continue;
            const auto leaf = key.substr(prefix.size());
            if(!leaf.empty() && leaf.find('/') == std::string::npos)
                lister->Add(ttstr{ leaf });
        }
    }

    void PSBMedia::GetLocallyAccessibleName(ttstr &name) {
        G_PluginLog.error("can't get GetLocallyAccessibleName from {}!",
                          name.AsStdString());
    }

    void PSBMedia::add(const std::string &name,
                       const std::shared_ptr<PSBResource> &resource) {
        this->_resources[name] = *resource;
    }
} // namespace PSB
