//
// Created by lidong on 25-6-21.
//

#include <catch2/catch_session.hpp>

#include "log/TVPInitLog.h"

#if defined(KRKR2_TEST_STANDALONE_TJS)
#include "tjsString.h"
// Standalone TJS tests have no engine locale manager.
TJS::ttstr TVPGetMessageByLocale(const std::string &key) {
    return TJS::ttstr(key);
}
#endif

int main(int argc, char *argv[]) {
    TVPInitLogging();

    int result = Catch::Session().run(argc, argv);

    return result;
}
