#pragma once

#include "TextInput.h"
#include <jni.h>
#include <memory>

namespace krkr::androidinput {
    // GetStringUTFChars returns JNI Modified UTF-8 (including CESU-8 surrogate
    // encodings), which must never enter the strict UTF-8 game input path.
    // The returned value owns its bytes before the caller queues engine work.
    inline std::optional<std::string> readCommittedText(JNIEnv *env,
                                                        jstring text) {
        if(!text)
            return std::nullopt;
        const auto length = env->GetStringLength(text);
        const auto *chars = env->GetStringChars(text, nullptr);
        if(!chars)
            return std::nullopt; // Keep a pending JVM allocation exception.
        const auto release = [env, text](const jchar *value) {
            env->ReleaseStringChars(text, value);
        };
        const std::unique_ptr<const jchar, decltype(release)> held(chars,
                                                                   release);
        std::u16string units;
        units.reserve(length);
        for(jsize i = 0; i < length; ++i)
            units.push_back(static_cast<char16_t>(chars[i]));
        return input::encodeTextInput(units);
    }
} // namespace krkr::androidinput
