/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or
 * computer language in any form
 * by any means, electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung
 * Electronics.
 */

#ifndef PNM_LOG_H
#define PNM_LOG_H

#include "pnmlib/common/definitions.h"

#include <fmt/core.h>

#include <spdlog/cfg/env.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

// We have 3 logging sinks:
//   1. "main" - for "info" and "debug" messages
//   2. "err" - for "error" messages, prints to stderr
//   3. "profile" - for "profile" messages
//
// Profiling messages are disabled by default, can be enabled in runtime.
//
// And in debug mode "debug" messages are enabled, otherwise disabled
//
// Everything is configurable at runtime by environment variables:
//   1. "SPDLOG_LEVEL" - disable/enable different messages
//   2. "PNM_LOG_VERBOSE" - optional extra verbose output
//
// --- Examples ---
//
// Increase verbosity:
//   PNM_LOG_VERBOSE=1
//   PNM_LOG_VERBOSE=true
//
// Enable profiling (disabled by default):
//   SPDLOG_LEVEL=profile=info // note that any level at all will enable it
//
// Disable everything except for errors:
//   SPDLOG_LEVEL=main=off
//
// Disable everything:
//   SPDLOG_LEVEL=off
//
// Disable everything except for errors and profiling:
//   SPDLOG_LEVEL=main=off,profiling=info
//
// Enable debug (disable in release build by default):
//   SPDLOG_LEVEL=main=debug

namespace pnm::log {

// Do this to remove explicit dependency on spdlog from user code
namespace level {

using namespace spdlog::level;

} // namespace level

namespace details {

class Context {
public:
  static Context &instance() {
    static Context instance;
    return instance;
  }

  /** @brief Set global logging level - applies to all types of logging messages
   */
  void set_active_level(spdlog::level::level_enum level) {
    spdlog::set_level(level);
  }

  /** @brief Enable or disable extra verbose mode (default = off)
   */
  void set_verbose(bool verbose) {
    const std::unordered_map<std::string, std::string> base_patterns = {
        {"main", "[pnmlib %l] %v"},
        {"profile", "[pnmlib profile] %v"},
        {"err", "[pnmlib %l][%@] %v"},
    };

    const char *verbose_prefix = "[%H:%M:%S.%e][pid %P][tid %t]";

    for (auto &[name, logger] : loggers_) {
      logger->set_pattern(
          verbose ? fmt::format("{}{}", verbose_prefix, base_patterns.at(name))
                  : base_patterns.at(name));
    }
  }

  std::shared_ptr<spdlog::logger> get_main_logger() { return loggers_["main"]; }
  std::shared_ptr<spdlog::logger> get_profile_logger() {
    return loggers_["profile"];
  }
  std::shared_ptr<spdlog::logger> get_err_logger() { return loggers_["err"]; }

private:
  Context() {
    loggers_["main"] = spdlog::stdout_logger_mt("main");
    loggers_["profile"] = spdlog::stdout_logger_mt("profile");
    loggers_["err"] = spdlog::stderr_logger_mt("err");

    set_verbose(get_verbose_from_env());

    auto default_level = DEBUG ? spdlog::level::debug : spdlog::level::info;

    set_active_level(default_level);
    // Disable profiling messages by default. They can be enabled at runtime,
    // see above
    loggers_["profile"]->set_level(level::off);

    spdlog::cfg::load_env_levels();
  }

  bool get_verbose_from_env() const {
    const char *env = std::getenv("PNM_LOG_VERBOSE");

    if (env == nullptr) {
      return false;
    }

    auto str = std::string(env);

    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return str == "1" || str == "true";
  }

  std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers_;
};

} // namespace details

inline void set_active_level(spdlog::level::level_enum level) {
  details::Context::instance().set_active_level(level);
}

inline void set_verbose(bool verbose) {
  details::Context::instance().set_verbose(verbose);
}

template <typename... Args>
inline void info(fmt::format_string<Args...> fmt, Args &&...args) {
  details::Context::instance().get_main_logger()->template info(
      fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void profile(fmt::format_string<Args...> fmt, Args &&...args) {
  // NOTE: Log at any level not equal to "off"
  details::Context::instance().get_profile_logger()->template critical(
      fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void debug(fmt::format_string<Args...> fmt, Args &&...args) {
  details::Context::instance().get_main_logger()->template debug(
      fmt, std::forward<Args>(args)...);
}

} // namespace pnm::log

#define PNM_LOG_ERROR(...)                                                     \
  SPDLOG_LOGGER_ERROR(pnm::log::details::Context::instance().get_err_logger(), \
                      __VA_ARGS__)

#endif
