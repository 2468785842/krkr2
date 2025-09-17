//
// Created by LiDon on 2025/9/11.
//
#pragma once

#include "PSBValue.h"
#include "StorageIntf.h"

namespace PSB {
    class PSBMedia : public iTVPStorageMedia {
    public:
        PSBMedia() { _ref = 1; }

        ~PSBMedia() override = default;

        void AddRef() override { _ref++; }

        void Release() override {
            if(_ref == 1)
                delete this;
            else
                _ref--;
        }

        void GetName(ttstr &name) override { name = TJS_W("psb"); }

        void NormalizeDomainName(ttstr &name) override;

        void NormalizePathName(ttstr &name) override;

        bool CheckExistentStorage(const ttstr &name) override;

        tTJSBinaryStream *Open(const ttstr &name, tjs_uint32 flags) override;

        void GetListAt(const ttstr &name, iTVPStorageLister *lister) override;

        void GetLocallyAccessibleName(ttstr &name) override;

        void add(const std::string &name,
                 const std::shared_ptr<PSBResource> &resource);

    private:
        int _ref = 0;
        std::unordered_map<std::string, PSBResource> _resources;
    };
} // namespace PSB