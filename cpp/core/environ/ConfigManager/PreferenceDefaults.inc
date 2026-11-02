// Shared by runtime initialization and optional management UI metadata.
TVP_BOOL_DEFAULT(outputlog, true)
TVP_BOOL_DEFAULT(showfps, false)
TVP_STRING_DEFAULT(fps_limit, "60")
TVP_STRING_DEFAULT(renderer, "software")
TVP_STRING_DEFAULT(default_font, "")
TVP_BOOL_DEFAULT(force_default_font, false)
#ifdef CC_TARGET_OS_IPHONE
TVP_STRING_DEFAULT(memusage, "high")
#else
TVP_STRING_DEFAULT(memusage, "unlimited")
#endif
TVP_BOOL_DEFAULT(keep_screen_alive, true)
TVP_FLOAT_DEFAULT(vcursor_scale, 0.5f)
TVP_FLOAT_DEFAULT(menu_handler_opa, 0.15f)
TVP_BOOL_DEFAULT(remember_last_path, true)
#if defined(CC_TARGET_PLATFORM) && CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
TVP_BOOL_DEFAULT(hide_android_sys_btn, false)
#endif
TVP_STRING_DEFAULT(software_draw_thread, "0")
TVP_STRING_DEFAULT(software_compress_tex, "none")
#ifdef CC_TARGET_OS_IPHONE
TVP_BOOL_DEFAULT(GL_EXT_shader_framebuffer_fetch, true)
#else
TVP_BOOL_DEFAULT(GL_EXT_shader_framebuffer_fetch, false)
#endif
TVP_BOOL_DEFAULT(GL_ARM_shader_framebuffer_fetch, true)
TVP_BOOL_DEFAULT(GL_NV_shader_framebuffer_fetch, true)
TVP_BOOL_DEFAULT(GL_EXT_copy_image, false)
TVP_BOOL_DEFAULT(GL_OES_copy_image, false)
TVP_BOOL_DEFAULT(GL_ARB_copy_image, false)
TVP_BOOL_DEFAULT(GL_NV_copy_image, false)
TVP_BOOL_DEFAULT(GL_EXT_clear_texture, true)
TVP_BOOL_DEFAULT(GL_ARB_clear_texture, true)
TVP_BOOL_DEFAULT(GL_QCOM_alpha_test, true)
TVP_BOOL_DEFAULT(ogl_accurate_render, false)
TVP_STRING_DEFAULT(ogl_max_texsize, "0")
TVP_STRING_DEFAULT(ogl_compress_tex, "none")
