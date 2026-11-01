//
// Created by lidong on 25-6-21.
//

#include <catch2/catch_session.hpp>

#include "log/TVPInitLog.h"

int main(int argc, char *argv[]) {
    TVPInitLogging();

    int result = Catch::Session().run(argc, argv);

    return result;
}