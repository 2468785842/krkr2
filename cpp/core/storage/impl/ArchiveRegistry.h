#pragma once

#include "StorageIntf.h"

using TVPArchiveCreatorFn = tTVPArchive *(*)(const ttstr &name,
                                             tTJSBinaryStream *st,
                                             bool normalizeFileName);

void TVPRegisterArchiveCreator(TVPArchiveCreatorFn creator);
void TVPClearArchiveCreators();
