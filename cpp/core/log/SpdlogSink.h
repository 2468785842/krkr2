#pragma once

#include "TVPLog.h"

class SpdlogSink final : public ITVPSink {
public:
    void write(TVPLogLevel level, const char *category,
               const std::string &message) override;
};
