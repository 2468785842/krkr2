//---------------------------------------------------------------------------
// Unified logging API (core_log)
//---------------------------------------------------------------------------
#ifndef TVPLogH
#define TVPLogH

#include <fmt/format.h>

#include <memory>
#include <string>

enum class TVPLogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
};

struct ITVPSink {
    virtual void write(TVPLogLevel level, const char *category,
                       const std::string &message) = 0;
    virtual ~ITVPSink() = default;
};

void TVPLog(TVPLogLevel level, const char *category,
            const std::string &message);
void TVPRegisterLogSink(std::unique_ptr<ITVPSink> sink);
void TVPUnregisterAllLogSinks();

template <typename... Args>
void TVPLogFmt(const TVPLogLevel level, const char *category,
               fmt::format_string<Args...> fmt, Args &&...args) {
    TVPLog(level, category, fmt::format(fmt, std::forward<Args>(args)...));
}

class TVPCategoryLogger {
    const char *category_;

    void log(const TVPLogLevel level, const std::string &message) const {
        TVPLog(level, category_, message);
    }

public:
    explicit TVPCategoryLogger(const char *category) : category_(category) {}

    [[nodiscard]] const char *category() const { return category_; }

    void trace(const std::string &message) const {
        log(TVPLogLevel::Trace, message);
    }
    void debug(const std::string &message) const {
        log(TVPLogLevel::Debug, message);
    }
    void info(const std::string &message) const {
        log(TVPLogLevel::Info, message);
    }
    void warn(const std::string &message) const {
        log(TVPLogLevel::Warn, message);
    }
    void error(const std::string &message) const {
        log(TVPLogLevel::Error, message);
    }
    void critical(const std::string &message) const {
        log(TVPLogLevel::Fatal, message);
    }

    template <typename... Args>
    void trace(fmt::format_string<Args...> fmt, Args &&...args) const {
        TVPLogFmt(TVPLogLevel::Trace, category_, fmt,
                  std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(fmt::format_string<Args...> fmt, Args &&...args) const {
        TVPLogFmt(TVPLogLevel::Debug, category_, fmt,
                  std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(fmt::format_string<Args...> fmt, Args &&...args) const {
        TVPLogFmt(TVPLogLevel::Info, category_, fmt,
                  std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(fmt::format_string<Args...> fmt, Args &&...args) const {
        TVPLogFmt(TVPLogLevel::Warn, category_, fmt,
                  std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(fmt::format_string<Args...> fmt, Args &&...args) const {
        TVPLogFmt(TVPLogLevel::Error, category_, fmt,
                  std::forward<Args>(args)...);
    }

    template <typename... Args>
    void critical(fmt::format_string<Args...> fmt, Args &&...args) const {
        TVPLogFmt(TVPLogLevel::Fatal, category_, fmt,
                  std::forward<Args>(args)...);
    }
};

#define TVP_LOG_HANDLE_LEVEL(level)                                            \
    void level(const std::string &message) const { cat().level(message); }     \
    template <typename... Args>                                                \
    void level(fmt::format_string<Args...> fmt, Args &&...args) const {        \
        cat().level(fmt, std::forward<Args>(args)...);                         \
    }

// Lightweight handle — PluginLog.critical(...) vs TVPPluginLog().critical(...)
template <auto Fn>
struct TVPLogHandle {
    TVP_LOG_HANDLE_LEVEL(trace)
    TVP_LOG_HANDLE_LEVEL(debug)
    TVP_LOG_HANDLE_LEVEL(info)
    TVP_LOG_HANDLE_LEVEL(warn)
    TVP_LOG_HANDLE_LEVEL(error)
    TVP_LOG_HANDLE_LEVEL(critical)
private:
    [[nodiscard]] static decltype(auto) cat() { return Fn(); }
};

#undef TVP_LOG_HANDLE_LEVEL

// Define a custom log category (spdlog logger is created on first write).
#define TVP_DEFINE_LOG_CATEGORY(CategoryName, category_string)                 \
    inline TVPCategoryLogger(&TVP##CategoryName##Log()) {                      \
        static TVPCategoryLogger logger(category_string);                      \
        return logger;                                                         \
    }                                                                          \
    inline constexpr TVPLogHandle<(&TVP##CategoryName##Log)>                   \
        G_##CategoryName##Log {}

TVP_DEFINE_LOG_CATEGORY(Core, "core");
TVP_DEFINE_LOG_CATEGORY(Tjs2, "tjs2");
TVP_DEFINE_LOG_CATEGORY(Plugin, "plugin");
TVP_DEFINE_LOG_CATEGORY(Engine, "engine");
TVP_DEFINE_LOG_CATEGORY(Exception, "exception");

#endif
