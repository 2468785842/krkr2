#include "IFileBackend.h"
#include "MsgIntf.h"
#include "StorageIntf.h"

// The extractor reopens local archive handles through the selected file
// backend. It does not install the engine's storage media or archive registry.
tTJSBinaryStream *TVPCreateStream(const ttstr &name, tjs_uint32 flags) {
    if((flags & TJS_BS_ACCESS_MASK) != TJS_BS_READ)
        TVPThrowExceptionMessage(TVPCannotWriteToArchive);
    if(!TVPGetFileBackend())
        TVPInitFileBackend();
    auto stream = TVPGetFileBackend()->OpenRead(name);
    if(!stream)
        TVPThrowExceptionMessage(TVPCannotOpenStorage, name);
    return stream.release();
}
