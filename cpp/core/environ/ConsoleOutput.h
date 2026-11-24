#pragma once

#include "tjsCommHead.h"
#include <atomic>

namespace krkr::host {
    // Optional visual console observer. The unified logger owns terminal/file
    // output; a host with an overlay installs its static UI listener at
    // startup.
    using ConsoleListener = void (*)(const ttstr &, bool);
    inline std::atomic<ConsoleListener> consoleListener{ nullptr };

    inline void SetConsoleListener(ConsoleListener listener) {
        consoleListener.store(listener);
    }
    inline void DeliverConsoleLine(const ttstr &line, bool important) {
        if(const auto listener = consoleListener.load())
            listener(line, important);
    }
} // namespace krkr::host
