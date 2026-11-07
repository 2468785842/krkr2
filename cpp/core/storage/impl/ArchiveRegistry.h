#pragma once

#include "StorageIntf.h"

// st 在返回非空归档时转移给创建器；返回 nullptr 或抛异常时仍由调用方持有。
// 拒绝格式后注册器会重置位置，再交给下一个创建器。
using TVPArchiveCreatorFn = tTVPArchive *(*)(const ttstr &name,
                                             tTJSBinaryStream *st,
                                             bool normalizeFileName);

void TVPRegisterArchiveCreator(TVPArchiveCreatorFn creator);
void TVPClearArchiveCreators();
