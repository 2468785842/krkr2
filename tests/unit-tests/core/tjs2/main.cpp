//
// Created by lidong on 25-6-21.
//

#include <catch2/catch_session.hpp>

#include "log/TVPInitLog.h"

#if defined(_WIN32)
#include <cstdlib>
#include <crtdbg.h>
#endif

#if defined(KRKR2_TEST_STANDALONE_TJS)
#include "tjsString.h"
// Standalone TJS tests have no engine locale manager.
TJS::ttstr TVPGetMessageByLocale(const std::string &key) {
    return TJS::ttstr(key);
}
#endif

int main(int argc, char *argv[]) {
#if defined(_WIN32)
    // Keep CRT failures visible to CTest without blocking unattended runs on
    // a modal report dialog. Assertions and abort still fail the process.
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _CALL_REPORTFAULT);
#if defined(_DEBUG)
    for(const int type : { _CRT_WARN, _CRT_ERROR, _CRT_ASSERT }) {
        _CrtSetReportMode(type, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
    }
#endif
#endif
    TVPInitLogging();

    int result = Catch::Session().run(argc, argv);

    return result;
}
