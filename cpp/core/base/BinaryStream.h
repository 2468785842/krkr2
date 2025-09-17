#pragma once

#include <optional>

#include "StorageIntf.h"

extern tTJSBinaryStream *TVPCreateBinaryStreamForRead(const ttstr &name,
                                                      const ttstr &modestr);
extern tTJSBinaryStream *TVPCreateBinaryStreamForWrite(const ttstr &name,
                                                       const ttstr &modestr);

/**
 * \brief 通用模式数字解析函数
 *
 * \param mode: 模式字符串
 * \param key: 模式字符 ('c', 'z', 'o')
 * \param maxDigits: 解析的最大数字长度
 * \param defaultValue: 没指定模式返回默认值
 *
 * \return 如果找不到数字返回empty
 */
inline std::optional<std::uint32_t> parseModeNumber(const tjs_char *mode,
                                                    tjs_char key, int maxDigits,
                                                    int defaultValue) {
    if(const tjs_char *p = TJS_strchr(mode, key)) {
        // 没指定数字
        if(!(p[1] >= TJS_W('0') && p[1] <= TJS_W('9'))) {
            return {};
        }
        int value = 0;
        p++; // 跳过模式字符
        for(int i = 0;
            i < maxDigits && p[i] >= TJS_W('0') && p[i] <= TJS_W('9'); i++) {
            value = value * 10 + (p[i] - TJS_W('0'));
        }
        return value;
    }
    return defaultValue;
}
