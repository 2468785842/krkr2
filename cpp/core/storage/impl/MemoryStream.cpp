#include "tjsCommHead.h"

#include "MsgIntf.h"
#include "UtilStreams.h"

#include <cstring>

//---------------------------------------------------------------------------
// tTVPMemoryStream（自 UtilStreams.cpp 提取，归档与引擎共用）
//---------------------------------------------------------------------------
tTVPMemoryStream::tTVPMemoryStream() { Init(); }

//---------------------------------------------------------------------------
tTVPMemoryStream::tTVPMemoryStream(const void *block, tjs_uint size) {
    Init();
    Block = const_cast<void *>(block);
    if(!Block) {
        Block = Alloc(size);
        if(!Block)
            TVPThrowExceptionMessage(TVPInsufficientMemory);
    } else {
        Reference = true;
    }
    Size = size;
    AllocSize = size;
    CurrentPos = 0;
}

//---------------------------------------------------------------------------
tTVPMemoryStream::~tTVPMemoryStream() {
    if(Block && !Reference)
        Free(Block);
}

//---------------------------------------------------------------------------
tjs_uint64 tTVPMemoryStream::Seek(tjs_int64 offset, tjs_int whence) {
    tjs_int64 newpos;
    switch(whence) {
        case TJS_BS_SEEK_SET:
            if(offset >= 0 && offset <= static_cast<tjs_int64>(Size))
                CurrentPos = static_cast<tjs_uint>(offset);
            return CurrentPos;

        case TJS_BS_SEEK_CUR:
            newpos = offset + static_cast<tjs_int64>(CurrentPos);
            if(newpos >= 0) {
                const tjs_uint np = static_cast<tjs_uint>(newpos);
                if(np <= Size)
                    CurrentPos = np;
            }
            return CurrentPos;

        case TJS_BS_SEEK_END:
            newpos = offset + static_cast<tjs_int64>(Size);
            if(newpos >= 0) {
                const tjs_uint np = static_cast<tjs_uint>(newpos);
                if(np <= Size)
                    CurrentPos = np;
            }
            return CurrentPos;
    }
    return CurrentPos;
}

//---------------------------------------------------------------------------
tjs_uint tTVPMemoryStream::Read(void *buffer, tjs_uint read_size) {
    if(CurrentPos + read_size >= Size)
        read_size = Size - CurrentPos;

    memcpy(buffer, static_cast<tjs_uint8 *>(Block) + CurrentPos, read_size);
    CurrentPos += read_size;
    return read_size;
}

//---------------------------------------------------------------------------
tjs_uint tTVPMemoryStream::Write(const void *buffer, tjs_uint write_size) {
    if(Reference)
        TVPThrowExceptionMessage(TVPWriteError);

    const tjs_uint newpos = CurrentPos + write_size;
    if(newpos >= AllocSize) {
        tjs_uint onesize;
        if(AllocSize < 64 * 1024)
            onesize = 4 * 1024;
        else if(AllocSize < 512 * 1024)
            onesize = 16 * 1024;
        else if(AllocSize < 4096 * 1024)
            onesize = 256 * 1024;
        else
            onesize = 2024 * 1024;
        AllocSize += onesize;

        if(CurrentPos + write_size >= AllocSize)
            AllocSize = CurrentPos + write_size;

        Block = Realloc(Block, AllocSize);
        if(AllocSize && !Block)
            TVPThrowExceptionMessage(TVPInsufficientMemory);
    }

    memcpy(static_cast<tjs_uint8 *>(Block) + CurrentPos, buffer, write_size);
    CurrentPos = newpos;
    if(CurrentPos > Size)
        Size = CurrentPos;
    return write_size;
}

//---------------------------------------------------------------------------
void tTVPMemoryStream::SetEndOfStorage() {
    if(Reference)
        TVPThrowExceptionMessage(TVPWriteError);

    Size = CurrentPos;
    AllocSize = Size;
    Block = Realloc(Block, Size);
    if(Size && !Block)
        TVPThrowExceptionMessage(TVPInsufficientMemory);
}

//---------------------------------------------------------------------------
void tTVPMemoryStream::Clear() {
    if(Block && !Reference)
        Free(Block);
    Init();
}

//---------------------------------------------------------------------------
void tTVPMemoryStream::SetSize(tjs_uint size) {
    if(Reference)
        TVPThrowExceptionMessage(TVPWriteError);

    if(Size > size) {
        Size = size;
        AllocSize = size;
        Block = Realloc(Block, size);
        if(CurrentPos > Size)
            CurrentPos = Size;
        if(size && !Block)
            TVPThrowExceptionMessage(TVPInsufficientMemory);
    } else {
        AllocSize = size;
        Size = size;
        Block = Realloc(Block, size);
        if(size && !Block)
            TVPThrowExceptionMessage(TVPInsufficientMemory);
    }
}

//---------------------------------------------------------------------------
void tTVPMemoryStream::Init() {
    Block = nullptr;
    Reference = false;
    Size = 0;
    AllocSize = 0;
    CurrentPos = 0;
}

//---------------------------------------------------------------------------
void *tTVPMemoryStream::Alloc(size_t size) { return TJS_malloc(size); }

//---------------------------------------------------------------------------
void *tTVPMemoryStream::Realloc(void *orgblock, size_t size) {
    return TJS_realloc(orgblock, size);
}

//---------------------------------------------------------------------------
void tTVPMemoryStream::Free(void *block) { TJS_free(block); }
