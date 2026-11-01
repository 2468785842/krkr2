#include "tjsCommHead.h"

#include "StorageIntf.h"
#include "StorageEngineAccess.h"
#include "MsgIntf.h"
#include "DebugIntf.h"

//---------------------------------------------------------------------------
// 引擎版 TVPCreateStream（归档 + 存储介质）
//---------------------------------------------------------------------------
static tTJSBinaryStream *_TVPCreateStream(const ttstr &_name,
                                          tjs_uint32 flags) {
    tTJSCriticalSectionHolder cs_holder(TVP_GetCreateStreamCS());

    ttstr name;

    tjs_uint32 access = flags & TJS_BS_ACCESS_MASK;
    if(access == TJS_BS_WRITE)
        name = TVPNormalizeStorageName(_name);
    else
        name = TVPGetPlacedPath(_name);

    if(name.IsEmpty()) {
        if(access >= 1)
            TVPRemoveFromStorageCache(_name);
        TVPThrowExceptionMessage(TVPCannotOpenStorage, _name);
    }

    const tjs_char *sharp_pos = TJS_strchr(name.c_str(), TVPArchiveDelimiter);
    if(sharp_pos) {
        if((flags & TJS_BS_ACCESS_MASK) != TJS_BS_READ)
            TVPThrowExceptionMessage(TVPCannotWriteToArchive);

        ttstr arcname(name, (int)(sharp_pos - name.c_str()));

        tTVPArchive *arc;
        tTJSBinaryStream *stream;
        arc = TVP_ArchiveCacheGet(arcname);
        try {
            ttstr in_arc_name(sharp_pos + 1);
            tTVPArchive::NormalizeInArchiveStorageName(in_arc_name);
            stream = arc->CreateStream(in_arc_name);
        } catch(...) {
            arc->Release();
            if(access >= 1)
                TVPRemoveFromStorageCache(_name);
            throw;
        }
        if(access >= 1)
            TVPRemoveFromStorageCache(_name);
        arc->Release();
        return stream;
    }

    tTJSBinaryStream *stream;
    try {
        stream = TVP_StorageMediaOpen(name, flags);
    } catch(...) {
        if(access >= 1)
            TVPRemoveFromStorageCache(_name);
        throw;
    }
    if(access >= 1)
        TVPRemoveFromStorageCache(_name);
    return stream;
}

tTJSBinaryStream *TVPCreateStream(const ttstr &_name, tjs_uint32 flags) {
    try {
        return _TVPCreateStream(_name, flags);
    } catch(eTJSScriptException &e) {
        if(TJS_strchr(_name.c_str(), '#'))
            e.AppendMessage(
                TJS_W("[") +
                TVPFormatMessage(TVPFilenameContainsSharpWarn, _name) +
                TJS_W("]"));
        throw;
    } catch(eTJSScriptError &e) {
        if(TJS_strchr(_name.c_str(), '#'))
            e.AppendMessage(
                TJS_W("[") +
                TVPFormatMessage(TVPFilenameContainsSharpWarn, _name) +
                TJS_W("]"));
        throw;
    } catch(eTJSError &e) {
        if(TJS_strchr(_name.c_str(), '#'))
            e.AppendMessage(
                TJS_W("[") +
                TVPFormatMessage(TVPFilenameContainsSharpWarn, _name) +
                TJS_W("]"));
        throw;
    } catch(...) {
        if(TJS_strchr(_name.c_str(), '#'))
            TVPAddLog(TVPFormatMessage(TVPFilenameContainsSharpWarn, _name));
        throw;
    }
}
