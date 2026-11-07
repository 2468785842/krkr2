#pragma once

#include <string>

namespace TVPPreferenceDefaults {
#define TVP_BOOL_DEFAULT(name, value)                                          \
    inline constexpr bool value_##name = value;
#define TVP_FLOAT_DEFAULT(name, value)                                         \
    inline constexpr float value_##name = value;
#define TVP_STRING_DEFAULT(name, value)                                        \
    inline constexpr const char *value_##name = value;
#include "PreferenceDefaults.inc"
#undef TVP_BOOL_DEFAULT
#undef TVP_FLOAT_DEFAULT
#undef TVP_STRING_DEFAULT
} // namespace TVPPreferenceDefaults

// Config supplies the same GetValue/IsValueExist/SetValueInt contract as
// iSysConfigManager. Keeping policy separate permits tests without GL or UI.
template <typename Config>
void TVPApplyRuntimePreferenceDefaults(Config &config, bool adreno) {
    if(adreno && !config.IsValueExist("GL_EXT_shader_framebuffer_fetch"))
        config.SetValueInt("GL_EXT_shader_framebuffer_fetch", 0);
#define TVP_BOOL_DEFAULT(name, value)                                          \
    config.template GetValue<bool>(#name, TVPPreferenceDefaults::value_##name);
#define TVP_FLOAT_DEFAULT(name, value)                                         \
    config.template GetValue<float>(#name, TVPPreferenceDefaults::value_##name);
#define TVP_STRING_DEFAULT(name, value)                                        \
    config.template GetValue<std::string>(                                     \
        #name, TVPPreferenceDefaults::value_##name);
#include "PreferenceDefaults.inc"
#undef TVP_BOOL_DEFAULT
#undef TVP_FLOAT_DEFAULT
#undef TVP_STRING_DEFAULT
}

// Call after GL creation, before game initialization, in both build modes.
void TVPInitializeRuntimePreferences();
