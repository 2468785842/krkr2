#pragma once

#include "JniStrings.h"
#include <SDL3/SDL.h>

namespace krkr::android {
    // Keep references scoped on the SDL thread, which is attached by SDL.
    class ActivityCall {
        JNIEnv *const env = static_cast<JNIEnv *>(SDL_GetAndroidJNIEnv());
        const bool localFrame = env && env->PushLocalFrame(32) == 0;
        const jobject activity = localFrame
            ? static_cast<jobject>(SDL_GetAndroidActivity())
            : nullptr;

    public:
        ~ActivityCall() {
            if(localFrame)
                env->PopLocalFrame(nullptr);
        }
        explicit operator bool() const { return activity != nullptr; }
        JNIEnv *Environment() const { return env; }
        jobject Activity() const { return activity; }

        jmethodID Method(const char *name, const char *signature) const {
            if(!activity)
                return nullptr;
            auto type = env->GetObjectClass(activity);
            auto method = env->GetMethodID(type, name, signature);
            env->DeleteLocalRef(type);
            Check();
            return method;
        }

        void Check() const {
            if(env->ExceptionCheck()) {
                env->ExceptionDescribe();
                env->ExceptionClear();
                throw std::runtime_error("Android activity call failed");
            }
        }
    };
} // namespace krkr::android
