#include <catch2/catch_session.hpp>

#include "HeadlessRuntime.h"
#include "io/LocalFileStream.h"
#include "log/TVPInitLog.h"
#include "msg/MsgIntf.h"
#include "storage/impl/ArchiveHandleCache.h"
#if defined(KRKR2_ARCHIVE_TEST_XP3)
#include "event/EventIntf.h"
#include "log/TVPLog.h"
#endif

#include <algorithm>
#include <cstdio>
#include <utility>
#include <vector>

namespace {
    int liveStreams = 0;
    int filenameErrors = 0;

    auto &exitHandlers() {
        static std::vector<std::pair<tjs_int, void (*)()>> handlers;
        return handlers;
    }

#if defined(KRKR2_ARCHIVE_TEST_XP3)
    std::vector<tTVPCompactEventCallbackIntf *> compactHandlers;
#endif

    class CountedFile : public tTVPStdioFileStream {
    public:
        explicit CountedFile(FILE *file) : tTVPStdioFileStream(file) {
            ++liveStreams;
        }
        ~CountedFile() override { --liveStreams; }
    };
} // namespace

int TestLiveArchiveStreams() { return liveStreams; }
int TestArchiveFilenameErrors() { return filenameErrors; }

// 测试宿主提供真实的本地只读流，并统计其生命周期，不安装引擎存储介质。
tTJSBinaryStream *TVPCreateStream(const ttstr &name, tjs_uint32 flags) {
    if((flags & TJS_BS_ACCESS_MASK) != TJS_BS_READ)
        TVPThrowExceptionMessage(TVPCannotWriteToArchive);
    FILE *file = std::fopen(name.AsStdString().c_str(), "rb");
    if(!file)
        TVPThrowExceptionMessage(TVPCannotOpenStorage, name);
    return new CountedFile(file);
}

ttstr TVPGetMessageByLocale(const std::string &key) { return ttstr(key); }

int TVPShowSimpleMessageBox(const ttstr &, const ttstr &) {
    ++filenameErrors;
    return 0;
}

void TVPAddAtExitHandler(tjs_int priority, void (*handler)()) {
    exitHandlers().emplace_back(priority, handler);
}

#if defined(KRKR2_ARCHIVE_TEST_XP3)
void TVPAddLog(const ttstr &line, bool important) {
    TVPLog(important ? TVPLogLevel::Warn : TVPLogLevel::Info, "engine",
           line.AsStdString());
}
void TVPAddLog(const ttstr &line) { TVPAddLog(line, false); }
void TVPAddImportantLog(const ttstr &line) { TVPAddLog(line, true); }
void TVPAddCompactEventHook(tTVPCompactEventCallbackIntf *callback) {
    compactHandlers.push_back(callback);
}
#endif

int main(int argc, char **argv) {
    TVPInitLogging();
    int result = Catch::Session().run(argc, argv);
#if defined(KRKR2_ARCHIVE_TEST_XP3)
    for(auto *callback : compactHandlers)
        callback->OnCompact(TVP_COMPACT_LEVEL_MAX);
#endif
    // 与引擎一样执行退出回调，并在静态锁析构前检查所有句柄都已关闭。
    auto &handlers = exitHandlers();
    std::stable_sort(
        handlers.begin(), handlers.end(),
        [](const auto &a, const auto &b) { return a.first < b.first; });
    for(const auto &handler : handlers)
        handler.second();
    TVPShutdownArchiveHandleCache(); // 重复关闭应安全。
    if(liveStreams != 0) {
        std::fprintf(stderr, "Leaked archive streams: %d\n", liveStreams);
        return result ? result : 1;
    }
    return result;
}
