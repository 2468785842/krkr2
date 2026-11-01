#pragma once

#include "StorageIntf.h"
#include "tjsUtils.h"

// StorageIntf.cpp 内部状态，供同模块其他编译单元（如 CreateStream.cpp）访问
tTJSStaticCriticalSection &TVP_GetCreateStreamCS();
tTVPArchive *TVP_ArchiveCacheGet(const ttstr &name);
tTJSBinaryStream *TVP_StorageMediaOpen(const ttstr &name, tjs_uint32 flags);
