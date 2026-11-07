//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// 所有归档格式共用的文件句柄缓存（从 XP3 实现迁出）
//---------------------------------------------------------------------------

#include "tjsCommHead.h"
#include "ArchiveHandleCache.h"
#include "SysInitIntf.h"

//---------------------------------------------------------------------------
// tTVPXP3ArchiveHandleCache
//---------------------------------------------------------------------------
#define TVP_MAX_ARCHIVE_HANDLE_CACHE 8
static tjs_uint TVPArchiveHandleCacheAge = 0;
struct tTVPArchiveHandleCacheItem {
    void *Pointer;
    tTJSBinaryStream *Stream;
    tjs_uint Age;
};
//---------------------------------------------------------------------------
static tTVPArchiveHandleCacheItem *TVPArchiveHandleCachePool = nullptr;
static bool TVPArchiveHandleCacheInit = false;
static bool TVPArchiveHandleCacheShutdown = false;
static tTJSCriticalSection TVPArchiveHandleCacheCS;

//---------------------------------------------------------------------------
tTJSBinaryStream *TVPGetCachedArchiveHandle(void *pointer, const ttstr &name) {
    // get cached archive file handle from the pool
    if(TVPArchiveHandleCacheShutdown) {
        // the pool has shutdown
        return TVPCreateStream(name, TJS_BS_READ);
    }

    tTJSCriticalSectionHolder cs_holder(TVPArchiveHandleCacheCS);

    if(!TVPArchiveHandleCacheInit) {
        // initialize the pool
        TVPArchiveHandleCachePool =
            new tTVPArchiveHandleCacheItem[TVP_MAX_ARCHIVE_HANDLE_CACHE];
        for(tjs_int i = 0; i < TVP_MAX_ARCHIVE_HANDLE_CACHE; i++) {
            TVPArchiveHandleCachePool[i].Pointer = nullptr;
            TVPArchiveHandleCachePool[i].Stream = nullptr;
            TVPArchiveHandleCachePool[i].Age = 0;
        }
        TVPArchiveHandleCacheInit = true;
    }

    // linear search wiil be enough here because the
    // TVP_MAX_ARCHIVE_HANDLE_CACHE is relatively small
    for(tjs_int i = 0; i < TVP_MAX_ARCHIVE_HANDLE_CACHE; i++) {
        tTVPArchiveHandleCacheItem *item = TVPArchiveHandleCachePool + i;
        if(item->Stream && item->Pointer == pointer) {
            // found in the pool
            tTJSBinaryStream *stream = item->Stream;
            item->Stream = nullptr;
            return stream;
        }
    }

    // not found in the pool
    // simply create a stream and return it
    return TVPCreateStream(name, TJS_BS_READ);
}
//---------------------------------------------------------------------------
/*static*/ void TVPReleaseCachedArchiveHandle(void *pointer,
                                              tTJSBinaryStream *stream) {
    // release archive file handle
    if(TVPArchiveHandleCacheShutdown || !TVPArchiveHandleCacheInit) {
        delete stream;
        return;
    }

    tTJSCriticalSectionHolder cs_holder(TVPArchiveHandleCacheCS);

    // search empty cell in the pool
    tjs_uint oldest_age = 0;
    tjs_int oldest = 0;
    for(tjs_int i = 0; i < TVP_MAX_ARCHIVE_HANDLE_CACHE; i++) {
        tTVPArchiveHandleCacheItem *item = TVPArchiveHandleCachePool + i;
        if(item->Stream == nullptr) {
            // found the empty cell; fill it
            item->Pointer = pointer;
            item->Stream = stream;
            item->Age = ++TVPArchiveHandleCacheAge;
            // counter overflow in TVPArchiveHandleCacheAge
            // is not so a big problem.
            // counter overflow can worsen the cache performance,
            // but it occurs only when the counter is overflowed
            // (it's too far less than usual)
            return;
        }

        if(i == 0 || oldest_age > item->Age) {
            oldest_age = item->Age;
            oldest = i;
        }
    }

    // empty cell not found
    // free oldest cell and fill it
    tTVPArchiveHandleCacheItem *oldest_item =
        TVPArchiveHandleCachePool + oldest;
    delete oldest_item->Stream, oldest_item->Stream = nullptr;
    oldest_item->Pointer = pointer;
    oldest_item->Stream = stream;
    oldest_item->Age = ++TVPArchiveHandleCacheAge;
}
//---------------------------------------------------------------------------
/*static*/ void TVPFreeArchiveHandlePoolByPointer(void *pointer) {
    // free all streams which have specified pointer
    if(TVPArchiveHandleCacheShutdown)
        return;
    if(!TVPArchiveHandleCacheInit)
        return;

    tTJSCriticalSectionHolder cs_holder(TVPArchiveHandleCacheCS);

    for(tjs_int i = 0; i < TVP_MAX_ARCHIVE_HANDLE_CACHE; i++) {
        tTVPArchiveHandleCacheItem *item = TVPArchiveHandleCachePool + i;
        if(item->Stream && item->Pointer == pointer) {
            delete item->Stream, item->Stream = nullptr;
            item->Pointer = nullptr;
            item->Age = 0;
        }
    }
}

//---------------------------------------------------------------------------
void TVPFreeArchiveHandlePool() {
    // free all streams
    if(TVPArchiveHandleCacheShutdown)
        return;
    if(!TVPArchiveHandleCacheInit)
        return;

    tTJSCriticalSectionHolder cs_holder(TVPArchiveHandleCacheCS);

    for(tjs_int i = 0; i < TVP_MAX_ARCHIVE_HANDLE_CACHE; i++) {
        tTVPArchiveHandleCacheItem *item = TVPArchiveHandleCachePool + i;
        if(item->Stream) {
            delete item->Stream, item->Stream = nullptr;
            item->Pointer = nullptr;
            item->Age = 0;
        }
    }
}

//---------------------------------------------------------------------------
void TVPShutdownArchiveHandleCache() {
    // free all stream and shutdown the pool
    tTJSCriticalSectionHolder cs_holder(TVPArchiveHandleCacheCS);

    if(TVPArchiveHandleCacheShutdown)
        return;
    TVPArchiveHandleCacheShutdown = true;
    if(!TVPArchiveHandleCacheInit)
        return;

    for(tjs_int i = 0; i < TVP_MAX_ARCHIVE_HANDLE_CACHE; i++) {
        if(TVPArchiveHandleCachePool[i].Stream)
            delete TVPArchiveHandleCachePool[i].Stream;
    }
    delete[] TVPArchiveHandleCachePool;
    TVPArchiveHandleCachePool = nullptr;
    TVPArchiveHandleCacheInit = false;
}

//---------------------------------------------------------------------------
static tTVPAtExit TVPShutdownArchiveCacheAtExit(TVP_ATEXIT_PRI_CLEANUP,
                                                TVPShutdownArchiveHandleCache);
//---------------------------------------------------------------------------
