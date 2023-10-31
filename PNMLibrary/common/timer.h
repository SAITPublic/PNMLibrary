/*
 * Copyright (C) 2021 Samsung Electronics Co. LTD
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

#ifndef PNM_TIMER_H
#define PNM_TIMER_H

#include "profile.h"

#include "common/log.h"

#include <optional>
#include <string>
#include <utility>

namespace pnm::utils {

using pnm::profile::get_profiling_flag;
using pnm::profile::ProfileInfo;

class NamedTimer {

  std::string name_;

public:
  explicit NamedTimer(std::string name) : name_(std::move(name)) {}

  void start() {
    if (get_profiling_flag()) {
      ProfileInfo::profiling_info()[name_].start_time();
    }
  }

  void end() {
    if (get_profiling_flag()) {
      ProfileInfo::profiling_info()[name_].end_time();
    }
  }

  static void print_profiling_info() {
    if (get_profiling_flag()) {
      for (const auto &data : ProfileInfo::profiling_info()) {
        pnm::log::profile("Action: {}", data.first);
        data.second.print_profile();
      }
    }
  }
};

// A named timer that is inactive if created when profiling is disabled.
class ProfileOnlyNamedTimer {
public:
  explicit ProfileOnlyNamedTimer(const char *name) {
    if (get_profiling_flag()) {
      timer_.emplace(name);
    }
  }

  void start() {
    if (timer_) {
      timer_->start();
    }
  }

  void end() {
    if (timer_) {
      timer_->end();
    }
  }

private:
  std::optional<NamedTimer> timer_;
};

template <typename Timer> class ScopedTimer {
  Timer timer_;

public:
  explicit ScopedTimer(const char *name) : timer_(name) { timer_.start(); }

  static void print_profiling_info() { NamedTimer::print_profiling_info(); }

  ~ScopedTimer() { timer_.end(); }
};

} // namespace pnm::utils

#define MEASURE_TIME()                                                         \
  const pnm::utils::ScopedTimer<pnm::utils::ProfileOnlyNamedTimer>             \
      _measure_time_##__COUNTER__(__FUNCTION__);

#define MEASURE_START(name)                                                    \
  pnm::utils::ProfileOnlyNamedTimer timer(name);                               \
  timer.start();

#define MEASURE_END(name)                                                      \
  pnm::utils::ProfileOnlyNamedTimer timer(name);                               \
  timer.end();

#endif // PNM_TIMER_H
