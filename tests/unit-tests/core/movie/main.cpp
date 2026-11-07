//
// Created by lidong on 25-6-21.
//

#include <catch2/catch_session.hpp>

#include "log/TVPInitLog.h"
#include "tjsString.h"

// This initialization-only host has no engine locale manager. Like the
// standalone TJS tests, preserve the message key instead of a window backend.
TJS::ttstr TVPGetMessageByLocale(const std::string &key) {
    return TJS::ttstr(key);
}

int main(int argc, char *argv[]) {
    TVPInitLogging();

    int result = Catch::Session().run(argc, argv);

    return result;
}
