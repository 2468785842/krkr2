#include "LaunchOptions.h"
#include "DesktopSettings.h"

#include <stdexcept>

namespace krkr::launch {
    namespace {
        void setGame(LaunchOptions &options, const std::string &value) {
            if(options.game || value.empty())
                throw std::invalid_argument("Specify exactly one game path");
            options.game = std::filesystem::u8path(value);
        }

        EngineOption engineOption(const std::string &argument) {
            if(argument.size() < 2 || argument[0] != '-' || argument[1] == '-')
                throw std::invalid_argument(
                    "Expected an engine option: -name[=value]");
            const auto equal = argument.find('=');
            auto name = argument.substr(0, equal);
            if(name.size() < 2)
                throw std::invalid_argument("Empty engine option name");
            return { name,
                     equal == std::string::npos ? "yes"
                                                : argument.substr(equal + 1) };
        }
    } // namespace

    LaunchOptions Parse(const std::vector<std::string> &arguments,
                        DesktopHost defaultHost) {
        LaunchOptions options;
        options.host = defaultHost;
        bool engineOnly = false;
        bool hostSpecified = false;
        for(std::size_t i = 0; i < arguments.size(); ++i) {
            const auto &arg = arguments[i];
            if(engineOnly) {
                options.engineOptions.push_back(engineOption(arg));
            } else if(arg == "--") {
                engineOnly = true;
            } else if(arg == "--help" || arg == "-h") {
                options.help = true;
            } else if(arg == "--list-settings") {
                options.listSettings = true;
            } else if(arg == "--set") {
                if(i + 1 == arguments.size())
                    throw std::invalid_argument("Expected --set KEY=VALUE");
                AddDesktopSetting(options.settings, arguments[++i]);
            } else if(arg.rfind("--set=", 0) == 0) {
                AddDesktopSetting(options.settings, arg.substr(6));
            } else if(arg.rfind("--host=", 0) == 0) {
                if(hostSpecified)
                    throw std::invalid_argument("Specify at most one host");
                hostSpecified = true;
                if(arg == "--host=sdl3")
                    options.host = DesktopHost::SDL3;
                else
                    throw std::invalid_argument("Unknown desktop host: " +
                                                arg.substr(7));
            } else if(arg == "--game" || arg == "--xp3") {
                if(i + 1 == arguments.size() || arguments[i + 1].empty() ||
                   arguments[i + 1][0] == '-')
                    throw std::invalid_argument("Missing game path after " +
                                                arg);
                setGame(options, arguments[++i]);
            } else if(arg.rfind("--game=", 0) == 0 ||
                      arg.rfind("--xp3=", 0) == 0) {
                setGame(options, arg.substr(arg.find('=') + 1));
            } else if(arg == "-loglevel" || arg == "--loglevel") {
                if(i + 1 == arguments.size() || arguments[i + 1].empty() ||
                   arguments[i + 1][0] == '-')
                    throw std::invalid_argument("Missing log level");
                options.engineOptions.push_back(
                    { "-loglevel", arguments[++i] });
            } else if(arg.rfind("--loglevel=", 0) == 0) {
                options.engineOptions.push_back(
                    { "-loglevel", arg.substr(11) });
            } else if(arg.rfind("--", 0) == 0) {
                throw std::invalid_argument("Unknown launch option: " + arg);
            } else if(!arg.empty() && arg[0] == '-') {
                options.engineOptions.push_back(engineOption(arg));
            } else {
                setGame(options, arg);
            }
        }
        const auto renderer = options.settings.find("renderer");
        if(options.host == DesktopHost::SDL3 &&
           renderer != options.settings.end() && renderer->second != "software")
            throw std::invalid_argument("SDL3 requires renderer=software");
        return options;
    }

    std::filesystem::path ResolveTarget(const std::filesystem::path &input) {
        namespace fs = std::filesystem;
        if(input.empty())
            throw std::invalid_argument("Empty game path");
        const auto path = fs::absolute(input).lexically_normal();
        if(fs::is_directory(path)) {
            if(fs::is_regular_file(path / "startup.tjs"))
                return path / "";
            if(fs::is_regular_file(path / "data.xp3"))
                return path / "data.xp3";
            throw std::invalid_argument(
                "Game directory needs startup.tjs or data.xp3: " +
                path.u8string());
        }
        if(!fs::is_regular_file(path))
            throw std::invalid_argument(
                "Game path is not a regular file or directory: " +
                path.u8string());
        return path;
    }

    const char *Usage() {
        return "Usage: krkr2 (--game PATH | --xp3 PATH | PATH) [--set "
               "KEY=VALUE] "
               "[-- -name=value ...]\n"
               "PATH: game directory (startup.tjs or data.xp3), or a supported "
               "archive.\n"
               "  --host=NAME    Select sdl3 (the default host)\n"
               "  --set KEY=VALUE  Override a setting for this run "
               "(repeatable)\n"
               "  --list-settings  List desktop setting keys and values\n"
               "  -h, --help       Show this help without opening a window\n";
    }
} // namespace krkr::launch
