#pragma once

#include "StorageIntf.h"

// 各归档格式共用句柄缓存；归档销毁前清除其条目，宿主退出时显式关闭缓存。
tTJSBinaryStream *TVPGetCachedArchiveHandle(void *owner, const ttstr &name);
void TVPReleaseCachedArchiveHandle(void *owner, tTJSBinaryStream *stream);
void TVPFreeArchiveHandlePoolByPointer(void *owner);
void TVPFreeArchiveHandlePool();
void TVPShutdownArchiveHandleCache();
