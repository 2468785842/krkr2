#pragma once

#include "TVPLog.h"

// Apply -loglevel spec (e.g. "debug" or "tjs2:debug,plugin:warn").
// defaultMinLevel is used for categories not listed in the spec.
void TVPApplyLogLevelSpec(const char *spec, TVPLogLevel defaultMinLevel);

// Sync spdlog logger level for an existing category (no-op if not created yet).
void TVPApplySpdlogLoggerLevel(const char *category);

bool TVPIsLogEnabled(TVPLogLevel level, const char *category);
