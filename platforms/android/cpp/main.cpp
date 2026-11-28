#include "AndroidActivity.h"
#include "ConfigManager/PreferenceStorage.h"
#include "launch/DesktopLaunch.h"
#include "log/TVPInitLog.h"
#include "resources/LocalAssetReader.h"
#include "sdl3/NativeDialogs.h"
#include "sdl3/SdlHost.h"

#include <SDL3/SDL_main.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
extern "C" {
#include <libavcodec/jni.h>
}

namespace {
    class ApkAssets final : public krkr::assets::AssetReader {
        AAssetManager *const assets;
        krkr::assets::LocalAssetReader local{ {} };

        using Asset = std::unique_ptr<AAsset, decltype(&AAsset_close)>;
        Asset Open(const std::string &name) const {
            return { AAssetManager_open(assets, name.c_str(),
                                        AASSET_MODE_BUFFER),
                     AAsset_close };
        }

    public:
        explicit ApkAssets(AAssetManager *assets) : assets(assets) {}

        bool Exists(const std::string &name) const override {
            return std::filesystem::u8path(name).is_absolute()
                ? local.Exists(name)
                : bool(Open(name));
        }

        bool Read(const std::string &name,
                  const krkr::assets::Reader &reader) const override {
            if(std::filesystem::u8path(name).is_absolute())
                return local.Read(name, reader);
            auto asset = Open(name);
            if(!asset)
                return false;
            const auto size = AAsset_getLength64(asset.get());
            const auto *bytes = static_cast<const unsigned char *>(
                AAsset_getBuffer(asset.get()));
            if(size < 0 || (!bytes && size))
                return false;
            reader(bytes, static_cast<std::size_t>(size));
            return true;
        }
    };
} // namespace

int SDL_main(int, char **) {
    using namespace krkr::android;
    try {
        ActivityCall call;
        if(!call)
            throw std::runtime_error(
                "SDL did not provide the Android activity");
        auto *env = call.Environment();
        auto tokens = static_cast<jobjectArray>(env->CallObjectMethod(
            call.Activity(),
            call.Method("launchArguments", "()[Ljava/lang/String;")));
        call.Check();
        // SDL's default argv bridge uses Modified UTF-8. Take the UTF-16
        // strings directly so supplementary characters survive in paths.
        auto arguments = FromJava(env, tokens);
        arguments.insert(arguments.begin(), "krkr2");
        std::vector<char *> argv;
        for(auto &argument : arguments)
            argv.push_back(argument.data());
        const auto argc = static_cast<int>(argv.size());
        argv.push_back(nullptr);
        TVPInitLogging({ TVPLogInitLevel::Info, nullptr,
                         TVPExtractLogLevelFromArgv(argc, argv.data()) });

        auto javaAssets = env->CallObjectMethod(
            call.Activity(),
            call.Method("getAssets", "()Landroid/content/res/AssetManager;"));
        call.Check();
        auto *manager = AAssetManager_fromJava(env, javaAssets);
        const auto *files = SDL_GetAndroidInternalStoragePath();
        if(!manager || !files)
            throw std::runtime_error("Cannot open Android runtime storage");
        JavaVM *vm = nullptr;
        if(env->GetJavaVM(&vm) == JNI_OK)
            av_jni_set_java_vm(vm, nullptr);

        krkr::assets::ScopedReader assets(std::make_shared<ApkAssets>(manager));
        krkr::config::ScopedPreferenceStorage preferences(
            std::make_shared<krkr::config::LocalPreferenceStorage>(
                files, krkr::sdl::ReportPreferenceWriteFailure));
        const auto result =
            krkr::launch::PrepareDesktopLaunch(argc, argv.data());
        if(result < 0)
            return krkr::sdl::Run(krkr::launch::DesktopTarget());
        if(result != 0)
            krkr::sdl::ShowMessage(krkr::launch::DesktopLaunchError(), "KrKr2",
                                   {});
        return result;
    } catch(const std::exception &error) {
        __android_log_write(ANDROID_LOG_ERROR, "KrKr2", error.what());
        krkr::sdl::ShowMessage(error.what(), "KrKr2", {});
        return 1;
    }
}
