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

#ifndef PNM_PROFILE_H
#define PNM_PROFILE_H

#include "log.h"

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace pnm::profile {

// Usage:
//   TimerMsT Timer;
//   Timer.tick();
//
//   /* Polling */
//   poll_on_address(polling + sizeof(uint32_t));
//
//   Timer.tock();
//   pnm::log::info("polling time: {}", Timer.duration().count());
//
// [TODO: @a.korzun] Think if this timer can be moved out of the `pnm::profile`
// namespace, as it is usable not only for profiling. Clarify the contracts of
// this timer (e.g. whether multiple `tock()`s are allowed) in the comments and
// consider giving the `tick/tock` methods more intuitive names (if deemed
// necessary).
template <class TimeT = std::chrono::milliseconds,
          class ClockT = std::chrono::high_resolution_clock>
class Timer {
  using timep_t = typename ClockT::time_point;
  std::optional<timep_t> start_, end_;

public:
  void tick() {
    start_ = ClockT::now();
    end_ = std::nullopt;
  }

  void tock() {
    assert(start_);
    end_ = ClockT::now();
  }

  void reset() { start_ = end_ = std::nullopt; }

  template <class TT = TimeT> TT duration() const {
    assert(start_ && end_);
    return std::chrono::duration_cast<TT>(end_.value() - start_.value());
  }
};

using TimerMsT =
    Timer<std::chrono::milliseconds, std::chrono::high_resolution_clock>;
using TimerUsT =
    Timer<std::chrono::microseconds, std::chrono::high_resolution_clock>;

class ProfileInfo {
  int64_t total_time_;
  unsigned int counter_;
  pnm::profile::TimerUsT last_timer_;
  double sum_of_squares_time_;

public:
  ProfileInfo() : total_time_(0), counter_(0), sum_of_squares_time_(0) {}

  void start_time() { last_timer_.tick(); }

  void end_time() {
    last_timer_.tock();
    const int64_t time = last_timer_.duration().count();
    sum_of_squares_time_ += time * time;
    total_time_ += time;
    ++counter_;
    last_timer_.reset();
  }

  inline static auto &profiling_info() {
    static thread_local std::map<std::string, ProfileInfo> profiling_info = {};
    return profiling_info;
  }

  void print_profile() const {
    pnm::log::profile("\tTotal duration (in microseconds): {}", total_time_);
    auto sqr_time = static_cast<int64_t>(sqrt(sum_of_squares_time_ / counter_));
    pnm::log::profile("\tRoot-mean-square duration (in microseconds): {}",
                      sqr_time);
    pnm::log::profile("\tNumber of calls: {}", counter_);
    pnm::log::profile("\tAverage duration (in microseconds): {:.4f}",
                      (static_cast<double>(total_time_) / counter_));
  }
};

inline bool &get_profiling_flag() {
  static thread_local bool is_profiling = false;
  return is_profiling;
}

inline void start_profiling() {
  get_profiling_flag() = true;
  pnm::profile::ProfileInfo::profiling_info() = {};
}

inline void end_profiling() { get_profiling_flag() = false; }

} // namespace pnm::profile

#endif // PNM_PROFILE_H
