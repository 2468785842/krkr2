#pragma once

#include "IFileBackend.h"

//---------------------------------------------------------------------------
// xp3 工具专用：stdio + std::filesystem 本地只读后端
//---------------------------------------------------------------------------
class StdFileBackend : public IFileBackend {
public:
    std::unique_ptr<tTJSBinaryStream> Open(const ttstr &storageName,
                                           const ttstr &localName,
                                           tjs_uint32 flags) override;
    std::unique_ptr<tTJSBinaryStream> OpenRead(const ttstr &path) override;
    bool Exists(const ttstr &path) override;
};
