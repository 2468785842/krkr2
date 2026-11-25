#include "DesktopSettings.h"

#include <algorithm>
#include <charconv>
#include <iterator>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace krkr::launch {
    namespace {
        using Validator = bool (*)(std::string_view);
        struct Setting {
            std::string_view name;
            std::string_view allowed;
            Validator valid;
        };

        template <int Minimum, int Maximum>
        bool integer(std::string_view text) {
            int value = 0;
            const auto end = text.data() + text.size();
            const auto parsed = std::from_chars(text.data(), end, value);
            return parsed.ec == std::errc{} && parsed.ptr == end &&
                value >= Minimum && value <= Maximum;
        }

        bool fraction(std::string_view text) {
            std::istringstream input{ std::string(text) };
            input.imbue(std::locale::classic());
            float value = 0;
            return (input >> std::noskipws >> value) && input.eof() &&
                value >= 0 && value <= 1;
        }

        constexpr Setting settingsList[] = {
            { "outputlog", "0|1", nullptr },
            { "showfps", "0|1", nullptr },
            { "fps_limit", "positive integer",
              integer<1, std::numeric_limits<int>::max()> },
            { "renderer", "software", nullptr },
            { "default_font", "font path or empty",
              [](std::string_view) { return true; } },
            { "force_default_font", "0|1", nullptr },
            { "memusage", "unlimited|high|medium|low", nullptr },
            { "keep_screen_alive", "0|1", nullptr },
            { "virtual_mouse", "0|1", nullptr },
            { "vcursor_scale", "number from 0 to 1", fraction },
            { "menu_handler_opa", "number from 0 to 1", fraction },
            { "user_language", "locale code or empty",
              [](std::string_view) { return true; } },
            { "software_draw_thread", "integer from 0 to 8", integer<0, 8> },
            { "software_compress_tex", "none|halfline|lz4|lz4+tlg5", nullptr },
            { "ogl_accurate_render", "0|1", nullptr },
            { "ogl_max_texsize", "integer from 0 to 16384", integer<0, 16384> },
            { "ogl_compress_tex", "none|half|etc2|pvrtc", nullptr },
            { "GL_EXT_shader_framebuffer_fetch", "0|1", nullptr },
            { "GL_ARM_shader_framebuffer_fetch", "0|1", nullptr },
            { "GL_NV_shader_framebuffer_fetch", "0|1", nullptr },
            { "GL_EXT_copy_image", "0|1", nullptr },
            { "GL_OES_copy_image", "0|1", nullptr },
            { "GL_ARB_copy_image", "0|1", nullptr },
            { "GL_NV_copy_image", "0|1", nullptr },
            { "GL_EXT_clear_texture", "0|1", nullptr },
            { "GL_ARB_clear_texture", "0|1", nullptr },
            { "GL_QCOM_alpha_test", "0|1", nullptr },
        };

        bool isChoice(std::string_view choices, std::string_view value) {
            for(auto end = choices.find('|'); end != choices.npos;
                end = choices.find('|')) {
                if(choices.substr(0, end) == value)
                    return true;
                choices.remove_prefix(end + 1);
            }
            return choices == value;
        }
    } // namespace

    void AddDesktopSetting(std::map<std::string, std::string> &settings,
                           const std::string &assignment) {
        const auto equal = assignment.find('=');
        if(equal == std::string::npos || equal == 0)
            throw std::invalid_argument("Expected --set KEY=VALUE");
        const auto name = assignment.substr(0, equal);
        const std::string_view value(assignment.data() + equal + 1,
                                     assignment.size() - equal - 1);
        const auto *setting = std::find_if(
            std::begin(settingsList), std::end(settingsList),
            [&](const Setting &entry) { return entry.name == name; });
        if(setting == std::end(settingsList))
            throw std::invalid_argument("Unknown desktop setting: " + name);
        if(!(setting->valid ? setting->valid(value)
                            : isChoice(setting->allowed, value)))
            throw std::invalid_argument("Invalid value for " + name +
                                        ": expected " +
                                        std::string(setting->allowed));
        settings[name] = value;
    }

    std::string ListDesktopSettings() {
        std::string result;
        for(const auto &setting : settingsList) {
            result += setting.name;
            result += '\t';
            result += setting.allowed;
            result += '\n';
        }
        return result;
    }
} // namespace krkr::launch
