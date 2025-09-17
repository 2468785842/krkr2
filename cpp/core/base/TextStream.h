#pragma once
#include "StorageIntf.h"

iTJSTextReadStream *TVPCreateTextStreamForRead(const ttstr &name,
                                               const ttstr &mode);
iTJSTextWriteStream *TVPCreateTextStreamForWrite(const ttstr &name,
                                                 const ttstr &mode);

std::string checkTextEncoding(const void *buf, size_t size,
                              std::uint8_t &bomSize);

void TVPSetDefaultReadEncoding(const ttstr &encoding);

const tjs_char *TVPGetDefaultReadEncoding();