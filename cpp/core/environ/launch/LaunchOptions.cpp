#include "LaunchOptions.h"

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

    LaunchOptions Parse(const std::vector<std::string> &arguments) {
        LaunchOptions options;
        bool engineOnly = false;
        for(std::size_t i = 0; i < arguments.size(); ++i) {
            const auto &arg = arguments[i];
            if(engineOnly) {
                options.engineOptions.push_back(engineOption(arg));
            } else if(arg == "--") {
                engineOnly = true;
            } else if(arg == "--help" || arg == "-h") {
                options.help = true;
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

    const char *Usage(bool managementUI) {
        static constexpr const char *withUI =
            "Usage: krkr2 [--game PATH | --xp3 PATH | PATH] [-- -name=value "
            "...]\n"
            "PATH: game directory (startup.tjs or data.xp3), or a supported "
            "archive.\n"
            "Without a target, open the existing game selector.\n"
            "  -h, --help    Show this help without opening a window\n";
        static constexpr const char *withoutUI =
            "Usage: krkr2 (--game PATH | --xp3 PATH | PATH) [-- -name=value "
            "...]\n"
            "PATH: game directory (startup.tjs or data.xp3), or a supported "
            "archive.\n"
            "A game target is required; management UI is disabled.\n"
            "  -h, --help    Show this help without opening a window\n";
        return managementUI ? withUI : withoutUI;
    }
} // namespace krkr::launch
