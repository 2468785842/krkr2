#include "StdFileBackend.h"

#include "LocalFileStream.h"

#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

//---------------------------------------------------------------------------
std::unique_ptr<tTJSBinaryStream> StdFileBackend::Open(const ttstr &storageName,
                                                       const ttstr &localName,
                                                       tjs_uint32 flags) {
    if((flags & TJS_BS_ACCESS_MASK) != TJS_BS_READ)
        return nullptr;
    return OpenRead(localName.IsEmpty() ? storageName : localName);
}

//---------------------------------------------------------------------------
std::unique_ptr<tTJSBinaryStream> StdFileBackend::OpenRead(const ttstr &path) {
    const std::string native = path.AsNarrowStdString();
    FILE *file = fopen(native.c_str(), "rb");
    if(!file)
        return nullptr;
    return std::make_unique<tTVPStdioFileStream>(file);
}

//---------------------------------------------------------------------------
bool StdFileBackend::Exists(const ttstr &path) {
    std::error_code ec;
    return fs::exists(path.AsNarrowStdString(), ec);
}
