#pragma once

#include "TextInput.h"
#include <jni.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace krkr::android {
    inline std::string FromJava(JNIEnv *env, jstring value) {
        if(!value)
            return {};
        const auto size = env->GetStringLength(value);
        std::vector<jchar> units(size);
        if(size)
            env->GetStringRegion(value, 0, size, units.data());
        const auto utf8 =
            input::encodeTextInput(std::u16string(units.begin(), units.end()));
        if(!utf8)
            throw std::invalid_argument("Invalid UTF-16 text");
        return *utf8;
    }

    inline jstring ToJava(JNIEnv *env, std::string_view value) {
        const auto utf16 = input::decodeTextInput(value);
        if(!utf16)
            return env->NewStringUTF("Invalid UTF-8 text");
        const std::vector<jchar> units(utf16->begin(), utf16->end());
        return env->NewString(units.empty() ? nullptr : units.data(),
                              static_cast<jsize>(units.size()));
    }

    inline std::vector<std::string> FromJava(JNIEnv *env, jobjectArray array) {
        std::vector<std::string> result;
        if(!array)
            return result;
        const auto count = env->GetArrayLength(array);
        result.reserve(count);
        for(jsize i = 0; i < count; ++i) {
            auto value =
                static_cast<jstring>(env->GetObjectArrayElement(array, i));
            result.push_back(FromJava(env, value));
            env->DeleteLocalRef(value);
        }
        return result;
    }

    inline jobjectArray ToJava(JNIEnv *env,
                               const std::vector<std::string> &values) {
        auto type = env->FindClass("java/lang/String");
        if(!type)
            return nullptr;
        auto array = env->NewObjectArray(static_cast<jsize>(values.size()),
                                         type, nullptr);
        env->DeleteLocalRef(type);
        for(jsize i = 0; array && i < static_cast<jsize>(values.size()); ++i) {
            auto value = ToJava(env, values[i]);
            env->SetObjectArrayElement(array, i, value);
            env->DeleteLocalRef(value);
        }
        return array;
    }
} // namespace krkr::android
