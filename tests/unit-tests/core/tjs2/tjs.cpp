//
// Created by LiDong on 2025/8/28.
//

#include <fstream>
#include <catch2/catch_test_macros.hpp>

#include "DebugImpl.h"
#include "FontImpl.h"
#include "Platform.h"
#include "test_config.h"
#include "TextStream.h"
#include "tjsScriptBlock.h"

class TJSConsoleOutputDef final : public iTJSConsoleOutput {
public:
    void ExceptionPrint(const tjs_char *msg) override {
        spdlog::get("tjs2")->critical(ttstr{ msg }.AsStdString());
    }

    void Print(const tjs_char *msg) override {
        spdlog::get("tjs2")->info(ttstr{ msg }.AsStdString());
    }
} static iTJSConsoleOutputDef{};


TEST_CASE("exec tjs2 script") {
    const auto tvPScriptEngine = new tTJS();
    tvPScriptEngine->SetPPValue(TJS_W("krkr2"), 1);
    tvPScriptEngine->SetConsoleOutput(&iTJSConsoleOutputDef);

    // register some TVP classes/objects/functions/propeties
    tTJSVariant val;
    iTJSDispatch2 *dsp;
    iTJSDispatch2 *global = tvPScriptEngine->GetGlobalNoAddRef();

#define REGISTER_OBJECT(classname, instance)                                   \
    dsp = (instance);                                                          \
    val = tTJSVariant(dsp /*, dsp*/);                                          \
    dsp->Release();                                                            \
    global->PropSet(TJS_MEMBERENSURE | TJS_IGNOREPROP, TJS_W(#classname),      \
                    nullptr, &val, global);

    REGISTER_OBJECT(Debug, TVPCreateNativeClass_Debug());

#define SCRIPT(file_name)                                                      \
    ttstr tvPInitTJSScriptText{};                                              \
    auto *stream =                                                             \
        TVPCreateTextStreamForRead(TEST_FILES_PATH "/tjs2/" file_name, "");    \
    stream->Read(tvPInitTJSScriptText, 0);

    SECTION("exec test_class.tjs") {
        SCRIPT("test_class.tjs");
        tvPScriptEngine->ExecScript(tvPInitTJSScriptText);
    }

    SECTION("exec test_function.tjs") {
        SCRIPT("test_function.tjs");
        tvPScriptEngine->ExecScript(tvPInitTJSScriptText);
    }

    SECTION("exec test_misc.tjs") {
        SCRIPT("test_misc.tjs");
        tvPScriptEngine->ExecScript(tvPInitTJSScriptText);
    }

    SECTION("exec test_string.tjs") {
        SCRIPT("test_string.tjs");
        tvPScriptEngine->ExecScript(tvPInitTJSScriptText);
    }

    SECTION("exec test_variant.tjs") {
        SCRIPT("test_variant.tjs");
        tvPScriptEngine->ExecScript(tvPInitTJSScriptText);
    }

    SECTION("exec test_with.tjs") {
        SCRIPT("test_with.tjs");
        tvPScriptEngine->ExecScript(tvPInitTJSScriptText);
    }

    // SECTION("exec test.tjs") {
    //     SCRIPT("test.tjs");
    //     tvPScriptEngine->ExecScript(tvPInitTJSScriptText);
    // }

    tvPScriptEngine->Release();
}
