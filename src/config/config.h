#pragma once
// src/config/config.h — forward-declarations / free functions for config loading.

#include "bitcoin2max/config.h"
#include <string>

namespace bitcoin2max {

/// Load a Config from a .conf file at *path*.
/// Missing keys retain their default values; a missing file returns defaults.
Config loadConfig(const std::string& path);

/// Print all effective config values to stdout (for diagnostics).
void printConfig(const Config& cfg);

} // namespace bitcoin2max
