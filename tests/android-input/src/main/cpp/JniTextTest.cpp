#include "AndroidTextInput.h"
#include <utility>

namespace {
    std::optional<std::string> queued;
    jbyteArray bytes(JNIEnv *env, const std::optional<std::string> &text) {
        if(!text)
            return nullptr;
        auto result = env->NewByteArray(static_cast<jsize>(text->size()));
        if(result && !text->empty())
            env->SetByteArrayRegion(
                result, 0, static_cast<jsize>(text->size()),
                reinterpret_cast<const jbyte *>(text->data()));
        return result;
    }
} // namespace

extern "C" JNIEXPORT jbyteArray JNICALL
Java_org_tvp_kirikiri2_NativeText_read(JNIEnv *env, jclass, jstring text) {
    return bytes(env, krkr::androidinput::readCommittedText(env, text));
}

extern "C" JNIEXPORT void JNICALL
Java_org_tvp_kirikiri2_NativeText_stage(JNIEnv *env, jclass, jstring text) {
    queued = krkr::androidinput::readCommittedText(env, text);
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_org_tvp_kirikiri2_NativeText_take(JNIEnv *env, jclass) {
    auto text = std::exchange(queued, std::nullopt);
    return bytes(env, text);
}
