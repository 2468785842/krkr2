#include "SpdlogSink.h"

#include "TVPInitLog.h"

#include <spdlog/spdlog.h>

void SpdlogSink::write(TVPLogLevel level, const char *category,
                       const std::string &message) {
    if(!category)
        category = "core";

    auto logger = spdlog::get(category);
    if(!logger) {
        TVPEnsureSpdlogLogger(category);
        logger = spdlog::get(category);
        if(!logger)
            return;
    }

    switch(level) {
        case TVPLogLevel::Trace:
            logger->trace("{}", message);
            break;
        case TVPLogLevel::Debug:
            logger->debug("{}", message);
            break;
        case TVPLogLevel::Info:
            logger->info("{}", message);
            break;
        case TVPLogLevel::Warn:
            logger->warn("{}", message);
            break;
        case TVPLogLevel::Error:
            logger->error("{}", message);
            break;
        case TVPLogLevel::Fatal:
            logger->critical("{}", message);
            break;
    }
}
