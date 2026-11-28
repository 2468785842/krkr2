#include "tjsCommHead.h"
#include "AndroidActivity.h"
#include "EventIntf.h"
#include "MenuItemImpl.h"
#include "NativeDialogs.h"

#include <android/log.h>
#include <utility>

namespace {
    using namespace krkr::android;
    enum class DialogMode { Message, Text, Menu, Open, Save };

    std::pair<int, std::string>
    Dialog(DialogMode mode, const std::string &title, const std::string &text,
           const std::string &initial, const std::vector<std::string> &labels) {
        try {
            ActivityCall call;
            if(call) {
                auto *env = call.Environment();
                auto method = call.Method(
                    "showEngineDialog",
                    "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;"
                    "[Ljava/lang/String;)[Ljava/lang/String;");
                auto result = static_cast<jobjectArray>(env->CallObjectMethod(
                    call.Activity(), method, static_cast<jint>(mode),
                    ToJava(env, std::string_view(title)),
                    ToJava(env, std::string_view(text)),
                    ToJava(env, std::string_view(initial)),
                    ToJava(env, labels)));
                call.Check();
                auto values = FromJava(env, result);
                if(values.size() == 2)
                    return { std::stoi(values[0]), std::move(values[1]) };
            }
        } catch(const std::exception &error) {
            __android_log_write(ANDROID_LOG_ERROR, "KrKr2", error.what());
        }
        return { -1, {} };
    }

    class MenuClick final : public tTVPOnMenuItemClickInputEvent {
        tTJSVariant root;

    public:
        explicit MenuClick(tTJSNI_MenuItem *item) :
            tTVPOnMenuItemClickInputEvent(item),
            root(item->GetRootMenuItem()->GetOwnerNoAddRef(),
                 item->GetRootMenuItem()->GetOwnerNoAddRef()) {}
    };
} // namespace

extern "C" JNIEXPORT jboolean JNICALL
Java_org_github_krkr2_GameActivity_requestGameClose(JNIEnv *, jclass) {
    if(!SDL_WasInit(SDL_INIT_VIDEO))
        return JNI_FALSE;
    SDL_Event event{};
    event.type = SDL_EVENT_QUIT;
    return SDL_PushEvent(&event) ? JNI_TRUE : JNI_FALSE;
}

namespace krkr::sdl {
    // Dialogs are owned by the Java UI thread and dismissed on Activity pause.
    void PumpNativeDialogs() {}
    void CloseNativeDialogs() {}

    void ShowPopupMenu(tTJSNI_MenuItem *root) {
        for(auto *parent = root; parent;) {
            std::vector<std::string> labels;
            std::vector<tTJSNI_MenuItem *> actions;
            for(auto *entry : parent->GetChildren()) {
                auto *item = static_cast<tTJSNI_MenuItem *>(entry);
                if(!item->GetVisible() || !item->GetEnabled() ||
                   !item->GetOwnerNoAddRef())
                    continue;
                ttstr caption;
                item->GetCaption(caption);
                if(caption.IsEmpty() || caption == TJS_W("-") ||
                   caption == TJS_W("+"))
                    continue;
                labels.push_back((item->GetChecked() ? "✓ " : "") +
                                 caption.AsStdString());
                actions.push_back(item);
            }
            if(actions.empty())
                return;
            ttstr title;
            parent->GetCaption(title);
            const auto result =
                Dialog(DialogMode::Menu, title.AsStdString(), {}, {}, labels);
            if(result.first < 0 ||
               static_cast<std::size_t>(result.first) >= actions.size())
                return;
            parent = actions[result.first];
            if(parent->GetChildren().empty()) {
                TVPPostInputEvent(new MenuClick(parent));
                return;
            }
        }
    }

    std::string ShowFileSelector(const std::string &title,
                                 const std::string &filename,
                                 const std::string &directory, bool save) {
        return Dialog(save ? DialogMode::Save : DialogMode::Open, title,
                      directory, filename, {})
            .second;
    }

    int ShowMessage(const std::string &text, const std::string &caption,
                    const std::vector<std::string> &buttons) {
        return Dialog(DialogMode::Message, caption, text, {}, buttons).first;
    }

    int ShowInputBox(std::string &text, const std::string &caption,
                     const std::string &prompt,
                     const std::vector<std::string> &buttons) {
        auto result = Dialog(DialogMode::Text, caption, prompt, text, buttons);
        if(result.first == 0)
            text = std::move(result.second);
        return result.first;
    }
} // namespace krkr::sdl

void TVPShowPopMenu(tTJSNI_MenuItem *menu) { krkr::sdl::ShowPopupMenu(menu); }

std::string TVPShowFileSelector(const std::string &title,
                                const std::string &filename,
                                std::string directory, bool save) {
    return krkr::sdl::ShowFileSelector(title, filename, directory, save);
}

// SDL's pinned static Android build exposes this internal dispatch function
// within libkrkr2. It assigns keyboard focus and owns queued string storage.
extern "C" void SDL_SendKeyboardText(const char *text);

extern "C" JNIEXPORT void JNICALL
Java_org_libsdl_app_KrkrTextInput_commit(JNIEnv *env, jclass, jstring text) {
    try {
        const auto utf8 = krkr::android::FromJava(env, text);
        if(utf8.find('\0') == std::string::npos)
            SDL_SendKeyboardText(utf8.c_str());
    } catch(const std::exception &error) {
        __android_log_write(ANDROID_LOG_WARN, "KrKr2", error.what());
    }
}
