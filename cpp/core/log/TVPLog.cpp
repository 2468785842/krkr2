#include "TVPLog.h"
#include "TVPLogLevelConfig.h"

#include <vector>

namespace {

    std::vector<std::unique_ptr<ITVPSink>> &TVPLogSinks() {
        static std::vector<std::unique_ptr<ITVPSink>> sinks;
        return sinks;
    }

} // namespace

void TVPLog(TVPLogLevel level, const char *category,
            const std::string &message) {
    if(!category)
        category = "core";
    if(!TVPIsLogEnabled(level, category))
        return;

    for(const auto &sink : TVPLogSinks()) {
        if(sink)
            sink->write(level, category, message);
    }
}

void TVPRegisterLogSink(std::unique_ptr<ITVPSink> sink) {
    if(sink)
        TVPLogSinks().push_back(std::move(sink));
}

void TVPUnregisterAllLogSinks() { TVPLogSinks().clear(); }
