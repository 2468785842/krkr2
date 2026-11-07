#include "LocalFileBackend.h"

#include "StorageImpl.h"

//---------------------------------------------------------------------------
std::unique_ptr<tTJSBinaryStream>
LocalFileBackend::Open(const ttstr &storageName, const ttstr &localName,
                       tjs_uint32 flags) {
    return std::unique_ptr<tTJSBinaryStream>(
        new tTVPLocalFileStream(storageName, localName, flags));
}

//---------------------------------------------------------------------------
std::unique_ptr<tTJSBinaryStream>
LocalFileBackend::OpenRead(const ttstr &path) {
    return Open(path, path, TJS_BS_READ);
}

//---------------------------------------------------------------------------
bool LocalFileBackend::Exists(const ttstr &path) {
    return TVPCheckExistentLocalPath(path);
}
