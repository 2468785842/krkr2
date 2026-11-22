#include "DesktopLaunch.h"
#include "DesktopSettings.h"
#include "LaunchOptions.h"

#include "tjsCommHead.h"
#include "ConfigManager/GlobalConfigManager.h"
#include "SysInitIntf.h"
#include "tjsString.h"

#include <cstdio>
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <utility>

int TVPCheckArchive(const ttstr &localname);

namespace krkr::launch {
    namespace {
        std::string target;
        int exitCode = 0;
        bool useSDL3 = false;

        void fail(const std::string &message) {
            exitCode = 2;
            std::fprintf(stderr, "krkr2: %s\n", message.c_str());
            std::fflush(stderr);
        }
    } // namespace

    int PrepareDesktopLaunch(int argc, char **argv) {
        target.clear();
        useSDL3 = false;
        exitCode = 0;
        try {
            std::vector<std::string> arguments;
            for(int i = 1; i < argc; ++i)
                arguments.emplace_back(argv[i]);
            auto options = Parse(arguments);
            if(options.help) {
                std::fputs(Usage(), stdout);
#if KRKR2_ENABLE_SDL3_HOST
                std::fputs("  --host=sdl3   Use SDL3 with an explicit game "
                           "target (Linux)\n",
                           stdout);
#endif
                return 0;
            }
            if(options.listSettings) {
                std::fputs(ListDesktopSettings().c_str(), stdout);
                return 0;
            }
            useSDL3 = options.host == DesktopHost::SDL3;
            if(useSDL3) {
#if KRKR2_ENABLE_SDL3_HOST
                if(!options.game)
                    throw std::invalid_argument(
                        "--host=sdl3 requires --game PATH");
                options.settings.try_emplace("renderer", "software");
#else
                throw std::invalid_argument(
                    "SDL3 host is unavailable in this build");
#endif
            }
            if(!options.game) {
                fail("Desktop launch requires --game PATH");
                std::fputs(Usage(), stderr);
                return exitCode;
            }
            target = ResolveTarget(*options.game).generic_u8string();
            for(const auto &option : options.engineOptions)
                TVPSetCommandLine(ttstr(option.name).c_str(),
                                  ttstr(option.value));
            if(!ValidateDesktopArchive())
                return exitCode;
            iSysConfigManager::SetLaunchOverrides(std::move(options.settings));
            return -1;
        } catch(const std::exception &error) {
            fail(error.what());
            return exitCode;
        }
    }

    const std::string &DesktopTarget() { return target; }
    bool UseSDL3Host() { return useSDL3; }
    int DesktopExitCode() { return exitCode; }

    bool ValidateDesktopArchive() {
        if(target.empty())
            return true;
        try {
            if(std::filesystem::is_directory(std::filesystem::u8path(target))) {
                // Recheck after parsing, before scheduling the game.
                if(std::filesystem::is_regular_file(
                       std::filesystem::u8path(target) / "startup.tjs"))
                    return true;
            } else if(TVPCheckArchive(ttstr(target)) == 1) {
                return true;
            }
            fail("No startup.tjs in launch target: " + target);
        } catch(const std::exception &error) {
            fail(error.what());
        } catch(...) {
            fail("Cannot read launch target: " + target);
        }
        return false;
    }
} // namespace krkr::launch
