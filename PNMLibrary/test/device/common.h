/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#ifndef COMMON_H
#define COMMON_H

#include "wrapper.h"

#include "common/make_error.h"
#include "common/profile.h"

#include "pnmlib/core/buffer.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"

#include "pnmlib/common/error.h"

#include <gtest/gtest.h>
#include <gtest/internal/gtest-param-util.h>

#include <fmt/core.h>

#include <sched.h>
#include <signal.h> // NOLINT(modernize-deprecated-headers)
#include <stdlib.h> // NOLINT(modernize-deprecated-headers)
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <thread>
#include <vector>

template <typename Test>
inline std::string generate_param_name(
    const testing::TestParamInfo<typename Test::ParamType> &info) {
  switch (info.param) {
  case pnm::Device::Type::SLS_AXDIMM:
    return "SLS_AXDIMM";
  case pnm::Device::Type::SLS_CXL:
    return "SLS_CXL";
  case pnm::Device::Type::IMDB_CXL:
    return "IMDB_CXL";
  }

  throw pnm::error::make_inval("Unknown device type {}.",
                               static_cast<int>(info.param));
}

inline bool compute_unit_run(DeviceWrapper &child_dev, uint8_t compute_unit,
                             int hold_time_ms, int timeout_sec) {
  bool acquired = false;

  const int timeout_ms = timeout_sec * 1000;
  pnm::profile::TimerMsT timer;
  timer.tick();

  do {
    try {
      compute_unit = child_dev.acquire_compute_unit(compute_unit);
      acquired = true;
    } catch (...) {
    }
    timer.tock();
  } while (!acquired && timer.duration().count() < timeout_ms);

  if (acquired) {
    std::this_thread::sleep_for(std::chrono::milliseconds(hold_time_ms));
    EXPECT_NO_THROW(child_dev.release_compute_unit(compute_unit));
  }

  return acquired;
}

inline bool allocate_mem(const pnm::ContextHandler &ctx, uint64_t size,
                         std::vector<pnm::memory::Buffer<uint8_t>> &buffers) {
  // local num_user_objects is needed to divide size variable evenly into 4
  // ranks
  static constexpr auto buffers_count = 4;
  const auto table_size = size / buffers_count;

  try {
    std::generate_n(std::back_inserter(buffers), buffers_count,
                    [table_size, &ctx]() {
                      return pnm::memory::Buffer<uint8_t>(table_size, ctx);
                    });
  } catch (pnm::error::Base &e) {
    buffers.clear();
    fmt::print("Allocation failed. Reason: {}.\n", e.what());
    return false;
  }

  return true;
}

inline bool handle_children_exits(const std::vector<pid_t> &children_pids,
                                  const std::vector<bool> &check_mask) {
  bool result = true;
  const auto children_num = children_pids.size();

  for (size_t child_index = 0; child_index < children_num; ++child_index) {
    if (check_mask.at(child_index)) {
      int status{};
      waitpid(children_pids[child_index], &status, 0);

      if (WIFEXITED(status)) {
        result &= (WEXITSTATUS(status) != EXIT_FAILURE);
      } else {
        result = false;
      }
    }
  }

  return result;
}

inline bool
handle_killed_children_exits(const std::vector<pid_t> &children_pids,
                             const std::vector<bool> &check_mask) {
  bool result = true;
  const auto children_num = children_pids.size();

  for (size_t child_index = 0; child_index < children_num; ++child_index) {
    if (check_mask.at(child_index)) {
      int status{};
      waitpid(children_pids[child_index], &status, 0);

      if (WIFSIGNALED(status)) {
        result &= (WTERMSIG(status) == SIGKILL);
      } else {
        result = false;
      }
    }
  }
  return result;
}

inline std::optional<size_t> fork_children(std::vector<pid_t> &children_pids) {
  const auto children_num = children_pids.size();

  for (size_t child_index = 0; child_index < children_num; ++child_index) {
    if (auto pid = fork()) {
      EXPECT_NE(pid, -1);
      children_pids[child_index] = pid;
    } else {
      return child_index;
    }
  }

  return std::nullopt;
}

class Pipes {
public:
  explicit Pipes(size_t sz) : pipes_{sz} {}

  ~Pipes() { close_all(); }

  void close_all_except(size_t id) {
    for (size_t i = 0; i < pipes_.size(); i++) {
      if (i != id) {
        pipes_.at(i).close();
      }
    }
  }

  // pipes_.size() is non-existent index, so it'll close all pipes
  void close_all() { close_all_except(pipes_.size()); }

  bool send_msg(size_t id) { return pipes_.at(id).send_msg(); }

  bool wait_msg(size_t id) { return pipes_.at(id).wait_msg(); }

  bool broadcast_msg(const std::vector<bool> &check_mask) {
    bool res = true;
    assert(check_mask.size() == pipes_.size());

    for (size_t child_index = 0; child_index < pipes_.size(); ++child_index) {
      if (check_mask.at(child_index)) {
        res &= send_msg(child_index);
      }
    }
    return res;
  }

  bool collect_msg(const std::vector<bool> &check_mask) {
    bool res = true;
    assert(check_mask.size() == pipes_.size());

    for (size_t child_index = 0; child_index < pipes_.size(); ++child_index) {
      if (check_mask.at(child_index)) {
        res &= wait_msg(child_index);
      }
    }
    return res;
  }

private:
  class Pipe {
  public:
    Pipe() { EXPECT_NO_THROW(open()); }

    Pipe &operator=(const Pipe &other) = delete;

    Pipe(const Pipe &other) = delete;

    ~Pipe() { EXPECT_NO_THROW(close()); }

    void open() {
      if (is_open_) {
        return;
      }

      is_open_ = true;
      if (pipe(fds_.data()) != 0) {
        pnm::error::throw_from_errno(errno);
      }
    }

    void close() {
      if (!is_open_) {
        return;
      }

      is_open_ = false;
      if (::close(fds_[0]) != 0 || ::close(fds_[1]) != 0) {
        pnm::error::throw_from_errno(errno);
      }
    }

    bool send_msg() {
      if (!is_open_) {
        return false;
      }

      return write(fds_[1], &message, sizeof(message)) == sizeof(message);
    }

    bool wait_msg() {
      if (!is_open_) {
        return false;
      }

      int buffer = 0;
      return read(fds_[0], &buffer, sizeof(buffer)) == sizeof(buffer);
    }

  private:
    std::array<int, 2> fds_;
    bool is_open_ = false;
    // just an arbitrary value for pipes communication
    static constexpr int message = 42;
  };

  std::vector<Pipe> pipes_;
};

inline uint8_t get_compute_units(pnm::Device::Type type) {
  return DeviceWrapper(type).compute_unit_count();
}

#endif // COMMON_H
