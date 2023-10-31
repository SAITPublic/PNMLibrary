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

#include "common.h"
#include "wrapper.h"

#include "test/utils/pnm_fmt.h"

#include "pnmlib/core/buffer.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <signal.h> // NOLINT(modernize-deprecated-headers)
#include <sys/types.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <future>
#include <thread>
#include <utility>
#include <vector>

const auto PnmDeviceTypes =
    ::testing::Values(pnm::Device::Type::SLS, pnm::Device::Type::IMDB);

// - parent forks 2 children
// - child[0] allocates almost all memory (9/10 size) and communicates to parent
// - parent communicate child[1] to try get resources
// - child[1] tries to get allocation while child[0] still holds and check for
// failure and communicates it to parent
// - parent let child[0] exit without releasing of any resource (i.e. forget to
// release)
// - parent wait child[0] exit and then let child[1] try to allocate again
// - child[0] exits without releasing
// - child[1] tries again to get resources and check for success
class MPChildNotReleasedResourceItselfF
    : public ::testing::TestWithParam<pnm::Device::Type> {
  static constexpr auto children_num = 2;
  static constexpr auto greedy_child_idx = 0;
  static constexpr int child_rank_hold_ms = 100;
  static constexpr int child_rank_acq_timeout = 0;

public:
  void run(pnm::Device::Type device_type) {
    std::vector<bool> greed_mask(children_num);
    greed_mask.at(greedy_child_idx) = true;
    auto ungreed_mask = greed_mask;
    ungreed_mask.flip();

    /* enable resource cleanup if it was disabled, in order to test such
     * functionality
     */
    const bool was_enabled_cleanup = exchange_cleanup(true, device_type);

    std::vector<pid_t> children_pids(children_num);
    Pipes child_parent_pipes(children_num);
    Pipes parent_child_pipes(children_num);

    if (auto fork_res = fork_children(children_pids)) {
      auto child_index = *fork_res;
      DeviceWrapper child_dev(device_type);
      // align total size with allocator granularity
      const auto total_size = child_dev.memory_size() / 10 * 9;
      bool res = true;
      child_parent_pipes.close_all_except(child_index);
      parent_child_pipes.close_all_except(child_index);

      auto &context = child_dev.context();

      if (child_index == greedy_child_idx) {
        std::vector<pnm::memory::Buffer<uint8_t>> buffers;
        res &= allocate_mem(context, total_size, buffers);
        for (uint8_t unit = 0; unit < child_dev.compute_unit_count(); ++unit) {
          try {
            child_dev.acquire_compute_unit(unit);
          } catch (...) {
            res = false;
          }
        }
        // communicate allocation done to parent
        res &= child_parent_pipes.send_msg(child_index);
        // wait for parent to allow exit
        res &= parent_child_pipes.wait_msg(child_index);
        std::exit(res ? 0 : EXIT_FAILURE);
      } else {
        std::vector<pnm::memory::Buffer<uint8_t>> buffers;
        // wait for parent communicate prior trying to get resources
        res &= parent_child_pipes.wait_msg(child_index);
        // greedy child is alive and holds all resources
        const auto quarter_size = total_size / 4;
        res &= !allocate_mem(context, quarter_size, buffers);
        // communicate to parent after failed allocation, so parent will kill
        // greedy child
        res &= child_parent_pipes.send_msg(child_index);
        // wait greedy child death confirmed from parent
        res &= parent_child_pipes.wait_msg(child_index);
        // greedy child has finished and it's resources have been freed
        res &= allocate_mem(context, quarter_size, buffers);
        for (size_t unit = 0; unit < child_dev.compute_unit_count(); ++unit) {
          res &= compute_unit_run(child_dev, unit, child_rank_hold_ms,
                                  child_rank_acq_timeout);
        }

        std::exit(res ? 0 : EXIT_FAILURE);
      }
    } else {
      // wait for greedy child to confirm it's allocation
      EXPECT_TRUE(child_parent_pipes.wait_msg(greedy_child_idx));
      // communicate other children to try allocation
      EXPECT_TRUE(parent_child_pipes.broadcast_msg(ungreed_mask));
      // wait children communicate after allocation failure
      EXPECT_TRUE(child_parent_pipes.collect_msg(ungreed_mask));
      // let greedy child exit
      EXPECT_TRUE(parent_child_pipes.send_msg(greedy_child_idx));
      // handle greedy child exit
      EXPECT_TRUE(handle_children_exits(children_pids, greed_mask));
      // communicate children to try allocation after resources freed
      EXPECT_TRUE(parent_child_pipes.broadcast_msg(ungreed_mask));
      EXPECT_TRUE(handle_children_exits(children_pids, ungreed_mask));

      restore_cleanup(was_enabled_cleanup, device_type);
    }
  }
};

// - parent forks 5 children
// - greedy child allocates 9/10 size of ranks memory, messages it to parent and
// starts sleeping
// - 4 wait parent to communicate start trying allocation
// - 4 children try to allocate quarter of rank size and get fail and
// communicate to parent
// - parent waits 4 children messages, then kill greedy child
// - 4 children now able to allocate quarter rank size each
class MPChildKilledAllocReusedOtherChildF
    : public ::testing::TestWithParam<pnm::Device::Type> {
public:
  void run(pnm::Device::Type device_type) {
    constexpr int killed_child_idx = 0;
    constexpr auto children_num = 5;

    std::vector<bool> greed_mask(children_num);
    greed_mask.at(killed_child_idx) = true;
    auto ungreed_mask = greed_mask;
    ungreed_mask.flip();

    /* enable resource cleanup if it was disabled, in order to test such
     * functionality
     */
    const bool was_enabled_cleanup = exchange_cleanup(true, device_type);

    std::vector<pid_t> children_pids(children_num);
    Pipes child_parent_pipes(children_num);
    Pipes parent_child_pipes(children_num);

    if (auto fork_res = fork_children(children_pids)) {
      auto child_index = *fork_res;
      DeviceWrapper child_dev(device_type);
      // align total size with allocator granularity
      const auto total_size = child_dev.memory_size() / 10 * 9;
      bool res = true;
      child_parent_pipes.close_all_except(child_index);
      parent_child_pipes.close_all_except(child_index);

      auto context = pnm::make_context(device_type);
      if (child_index == killed_child_idx) {
        std::vector<pnm::memory::Buffer<uint8_t>> buffers;
        // NOTE: Here we must exit immediately on errors, as the process is
        // expected to be killed later
        if (!allocate_mem(context, total_size, buffers)) {
          std::exit(EXIT_FAILURE);
        }
        // communicate allocation done to parent
        if (!child_parent_pipes.send_msg(child_index)) {
          std::exit(EXIT_FAILURE);
        }
        // nothing to do more, just sleep untill parent kill this process
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
      } else {
        std::vector<pnm::memory::Buffer<uint8_t>> buffers;
        // wait for parent communicate prior trying to get resources
        res &= parent_child_pipes.wait_msg(child_index);
        // greedy child is alive and holds whole memory
        const auto quarter_size = total_size / 4;
        res &= !allocate_mem(context, quarter_size, buffers);
        // communicate to parent after failed allocation, so parent will kill
        // greedy child
        res &= child_parent_pipes.send_msg(child_index);
        // wait greedy child death confirmed from parent
        res &= parent_child_pipes.wait_msg(child_index);
        // greedy child has been killed by parent and it's resources have been
        // freed
        res &= allocate_mem(context, quarter_size, buffers);
      }
      std::exit(res ? 0 : EXIT_FAILURE);
    } else {
      // wait for greedy child to confirm it's allocation
      EXPECT_TRUE(child_parent_pipes.wait_msg(killed_child_idx));
      // communicate other children to try allocation
      EXPECT_TRUE(parent_child_pipes.broadcast_msg(ungreed_mask));
      // wait children communicate after allocation failure
      EXPECT_TRUE(child_parent_pipes.collect_msg(ungreed_mask));
      kill(children_pids[killed_child_idx], SIGKILL);
      // handle greedy child death
      EXPECT_TRUE(handle_killed_children_exits(children_pids, greed_mask));
      // communicate children to try allocation after resources freed
      EXPECT_TRUE(parent_child_pipes.broadcast_msg(ungreed_mask));
      EXPECT_TRUE(handle_children_exits(children_pids, ungreed_mask));

      restore_cleanup(was_enabled_cleanup, device_type);
    }
  }
};

// - parent forks N children
// - each child acquire each rank in a loop (starting from child index)
// - parent waits killable children to notify they acquired resource
// - parent kill appropriate children
// - test check that survived children are able to acquire/release all ranks
class MPChildKilledComputeUnitsReusedOtherChildF
    : public ::testing::TestWithParam<pnm::Device::Type> {
public:
  void run(pnm::Device::Type device_type) {
    constexpr int child_rank_hold_ms = 300;
    constexpr int child_rank_acq_timeout = 14;
    const auto children_num = get_compute_units(device_type);

    std::vector<bool> killed_children_mask(children_num);
    killed_children_mask.at(1) = true;
    killed_children_mask.at(2) = true;
    auto unkilled_children_mask = killed_children_mask;
    unkilled_children_mask.flip();

    /* enable resource cleanup if it was disabled, in order to test such
     * functionality
     */
    const bool was_enabled_cleanup = exchange_cleanup(true, device_type);

    std::vector<pid_t> children_pids(children_num);
    Pipes child_parent_pipes(children_num);
    Pipes parent_child_pipes(children_num);

    if (auto fork_res = fork_children(children_pids)) {
      auto child_index = *fork_res;
      auto child_dev = DeviceWrapper(device_type);
      bool res = true;
      child_parent_pipes.close_all_except(child_index);
      parent_child_pipes.close_all_except(child_index);
      if (killed_children_mask.at(child_index)) {
        // acquire rank
        try {
          child_dev.acquire_compute_unit(child_index);
        } catch (...) {
          res = false;
        }
        res &= child_parent_pipes.send_msg(child_index);
        // nothing to do more, just sleep until parent kill this process
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
      } else {
        res &= parent_child_pipes.wait_msg(child_index);
        for (auto i = 0; i < children_num; ++i) {
          // each child start to acquire the same rank as it index, so each
          // child will get resource, prior parent start killing some of them
          const uint8_t rank = (child_index + i) % children_num;
          res &= compute_unit_run(child_dev, rank, child_rank_hold_ms,
                                  child_rank_acq_timeout);
        }
      }

      std::exit(res ? 0 : EXIT_FAILURE);
    } else {
      // wait killable communicate
      EXPECT_TRUE(child_parent_pipes.collect_msg(killed_children_mask));
      for (int ch_idx = 0; ch_idx < children_num; ++ch_idx) {
        if (killed_children_mask.at(ch_idx)) {
          kill(children_pids[ch_idx], SIGKILL);
        }
      }
      EXPECT_TRUE(
          handle_killed_children_exits(children_pids, killed_children_mask));
      EXPECT_TRUE(parent_child_pipes.broadcast_msg(unkilled_children_mask));
      EXPECT_TRUE(handle_children_exits(children_pids, unkilled_children_mask));

      restore_cleanup(was_enabled_cleanup, device_type);
    }
  }
};

// - parent forks N children
// - parent waits until every child acquires a rank/thread
// - parent kill one child, other children are able to release rank and
// finish
// - parent forks children again
// - no children killed
// - test check that children of the same index as killed on first run are able
// to acquire rank
class MPChildKilledComputeUnitsReusedNextRunF
    : public ::testing::TestWithParam<pnm::Device::Type> {
public:
  void run(pnm::Device::Type device_type) {
    constexpr int child_rank_hold_ms = 500;
    constexpr int child_rank_acq_timeout = 14;
    const auto children_num = get_compute_units(device_type);

    std::vector<pid_t> children_pids(children_num);
    Pipes child_parent_pipes(children_num);
    Pipes parent_child_pipes(children_num);

    /* enable resource cleanup if it was disabled, in order to test such
     * functionality
     */
    const bool was_enabled_cleanup = exchange_cleanup(true, device_type);

    if (auto fork_res = fork_children(children_pids)) {
      auto child_index = *fork_res;
      DeviceWrapper child_dev(device_type);

      bool res = true;
      uint8_t compute_unit;
      try {
        compute_unit = child_dev.acquire_compute_unit(child_index);
      } catch (...) {
        res = false;
        std::exit(EXIT_FAILURE);
      }
      // child acquired rank
      res &= child_parent_pipes.send_msg(child_index);
      // wait a signal from parent to continue
      res &= parent_child_pipes.wait_msg(child_index);
      child_dev.release_compute_unit(compute_unit);
      std::exit(res ? 0 : EXIT_FAILURE);
    }

    const std::vector<bool> children_all(children_num, true);
    // wait for all children to acquire ranks
    child_parent_pipes.collect_msg(children_all);

    std::vector<bool> killed_children_mask(children_num);
    killed_children_mask.at(1) = true;
    for (int ch_idx = 0; ch_idx < children_num; ++ch_idx) {
      if (killed_children_mask.at(ch_idx)) {
        kill(children_pids[ch_idx], SIGKILL);
      }
    }

    auto alive_children_mask = killed_children_mask;
    alive_children_mask.flip();
    // send message to alive children to continue
    parent_child_pipes.broadcast_msg(alive_children_mask);
    EXPECT_TRUE(
        handle_killed_children_exits(children_pids, killed_children_mask));
    EXPECT_TRUE(handle_children_exits(children_pids, alive_children_mask));

    if (auto fork_res = fork_children(children_pids)) {
      auto child_index = *fork_res;
      DeviceWrapper child_dev(device_type);
      const bool res = compute_unit_run(
          child_dev, child_index, child_rank_hold_ms, child_rank_acq_timeout);
      std::exit(res ? 0 : EXIT_FAILURE);
    }
    EXPECT_TRUE(handle_children_exits(children_pids, children_all));

    restore_cleanup(was_enabled_cleanup, device_type);
  }
};

struct WorkerMarkers {
  std::future<void> acquired;
  std::promise<void> can_continue;
  std::future<bool> result;
};

WorkerMarkers launch_worker(DeviceWrapper &device, int worker_idx,
                            bool is_releasing) {
  std::promise<void> acquired_promise;
  auto acquired_future = acquired_promise.get_future();

  std::promise<void> continue_promise;
  auto continue_future = continue_promise.get_future();

  // Totally false-positive warning. clang-tidy doesn't like moving
  // std::promise inside a lambda for some reason
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
  auto func = [acquired_marker = std::move(acquired_promise),
               continue_marker = std::move(continue_future), &device,
               worker_idx, is_releasing]() mutable -> bool {
    bool acquired = true;

    uint8_t compute_unit{};

    try {
      compute_unit = device.acquire_compute_unit(worker_idx);
    } catch (...) {
      acquired = false;
    }

    acquired_marker.set_value();
    continue_marker.get();

    if (is_releasing && acquired) {
      try {
        device.release_compute_unit(compute_unit);
      } catch (...) {
        return false;
      }
    }

    return acquired;
  };

  auto result = std::async(std::launch::async, std::move(func));

  return {std::move(acquired_future), std::move(continue_promise),
          std::move(result)};
}

auto launch_workers(DeviceWrapper &device, int count, bool is_releasing) {
  std::vector<WorkerMarkers> result(count);

  for (int i = 0; i < count; ++i) {
    result[i] = launch_worker(device, i, is_releasing);
  }

  return result;
}

// - parent forks 2 children
// - child[0] worker acquires ranks and completes without release
// - child[1] worker runs after child[0] worker and checks that ranks are
// acquired successfully
class MTWorkerNotReleasedResourceItselfF
    : public ::testing::TestWithParam<pnm::Device::Type> {
public:
  void run(pnm::Device::Type device_type) {
    constexpr auto children_num = 2;
    constexpr int unreleased_child_idx = 0;
    const int workers_num = get_compute_units(device_type);

    std::vector<pid_t> children_pids(children_num);
    Pipes parent_to_child(1);

    /* enable resource cleanup if it was disabled, in order to test such
     * functionality */
    const bool was_enabled_cleanup = exchange_cleanup(true, device_type);

    if (auto fork_res = fork_children(children_pids)) {
      auto child_index = *fork_res;
      bool res = true;

      if (child_index == unreleased_child_idx) {
        {
          auto child_dev = DeviceWrapper(device_type);
          auto worker_markers = launch_workers(child_dev, workers_num, false);

          for (auto &marker : worker_markers) {
            marker.can_continue.set_value();
          }

          for (auto &marker : worker_markers) {
            res &= marker.result.get();
          }
        } // device closed and process manager cleanup resources

        std::exit(res ? 0 : EXIT_FAILURE);
      } else {
        parent_to_child.wait_msg(0);

        auto child_dev = DeviceWrapper(device_type);
        auto worker_markers = launch_workers(child_dev, workers_num, true);

        for (auto &marker : worker_markers) {
          marker.can_continue.set_value();
        }

        for (auto &marker : worker_markers) {
          res &= marker.result.get();
        }

        std::exit(res ? 0 : EXIT_FAILURE);
      }
    }

    std::vector<bool> zero_child(children_num);
    zero_child.at(0) = true;
    std::vector<bool> first_child(children_num);
    first_child.at(0) = true;
    first_child.at(1) = true;

    EXPECT_TRUE(handle_children_exits(children_pids, zero_child));
    // trigger first child to continue
    parent_to_child.send_msg(0);
    EXPECT_TRUE(handle_children_exits(children_pids, first_child));

    restore_cleanup(was_enabled_cleanup, device_type);
  }
};

// - parent forks 2 children
// - child[0] workers acquires ranks and got killed by parent
// - child[1] workers run after child[0] workers and check that ranks are
// acquired successfully
class MTWorkerNotReleasedResourceKilledF
    : public ::testing::TestWithParam<pnm::Device::Type> {
public:
  void run(pnm::Device::Type device_type) {
    constexpr auto children_num = 2;
    constexpr int killed_child_idx = 0;
    const int workers_num = get_compute_units(device_type);

    /* enable resource cleanup if it was disabled, in order to test such
     * functionality
     */
    const bool was_enabled_cleanup = exchange_cleanup(true, device_type);

    std::vector<pid_t> children_pids(children_num);
    Pipes child_parent_pipes(children_num);
    Pipes parent_child_pipes(children_num);

    if (auto fork_res = fork_children(children_pids)) {
      auto child_index = *fork_res;
      DeviceWrapper child_dev(device_type);
      bool res = true;
      child_parent_pipes.close_all_except(child_index);
      parent_child_pipes.close_all_except(child_index);

      if (child_index == killed_child_idx) {
        auto worker_markers = launch_workers(child_dev, workers_num, false);

        // just let all workers acquire rank
        for (auto &marker : worker_markers) {
          marker.acquired.get();
        }

        child_parent_pipes.send_msg(child_index);

        // The process should be killed here. But if it's not, then wait for
        // some time and exit with an error

        constexpr auto timeout = std::chrono::seconds(2);
        std::this_thread::sleep_for(timeout);

        res = false;

        for (auto &marker : worker_markers) {
          marker.can_continue.set_value();
        }

        for (auto &marker : worker_markers) {
          marker.result.get();
        }
      } else {
        res &= parent_child_pipes.wait_msg(child_index);

        auto worker_markers = launch_workers(child_dev, workers_num, true);

        for (auto &marker : worker_markers) {
          marker.can_continue.set_value();
        }

        for (auto &marker : worker_markers) {
          res &= marker.result.get();
        }
      }

      std::exit(res ? 0 : EXIT_FAILURE);
    } else {
      EXPECT_TRUE(child_parent_pipes.wait_msg(killed_child_idx));
      std::vector<bool> killed_children_mask(children_num);
      killed_children_mask.at(killed_child_idx) = true;
      auto unkilled_children_mask = killed_children_mask;
      unkilled_children_mask.flip();
      kill(children_pids[killed_child_idx], SIGKILL);
      EXPECT_TRUE(
          handle_killed_children_exits(children_pids, killed_children_mask));
      // though only one child waits for message, broadcast manages bits mask
      // handling
      EXPECT_TRUE(parent_child_pipes.broadcast_msg(unkilled_children_mask));
      EXPECT_TRUE(handle_children_exits(children_pids, unkilled_children_mask));

      restore_cleanup(was_enabled_cleanup, device_type);
    }
  }
};

TEST_P(MPChildNotReleasedResourceItselfF, MPChildNotReleasedResourceItself) {
  this->run(GetParam());
}
TEST_P(MPChildKilledAllocReusedOtherChildF,
       MPChildKilledAllocReusedOtherChild) {
  this->run(GetParam());
}
TEST_P(MPChildKilledComputeUnitsReusedOtherChildF,
       MPChildKilledRanksReusedOtherChild) {
  this->run(GetParam());
}
TEST_P(MPChildKilledComputeUnitsReusedNextRunF,
       MPChildKilledRanksReusedNextRun) {
  this->run(GetParam());
}
TEST_P(MTWorkerNotReleasedResourceItselfF, MTWorkerNotReleasedResourceItself) {
  this->run(GetParam());
}
TEST_P(MTWorkerNotReleasedResourceKilledF, MTWorkerNotReleasedResourceKilled) {
  this->run(GetParam());
}

INSTANTIATE_TEST_SUITE_P(PnmProcessManager, MPChildNotReleasedResourceItselfF,
                         PnmDeviceTypes, print_fmt_test_params);

INSTANTIATE_TEST_SUITE_P(PnmProcessManager, MPChildKilledAllocReusedOtherChildF,
                         PnmDeviceTypes, print_fmt_test_params);

INSTANTIATE_TEST_SUITE_P(PnmProcessManager,
                         MPChildKilledComputeUnitsReusedOtherChildF,
                         PnmDeviceTypes, print_fmt_test_params);

INSTANTIATE_TEST_SUITE_P(PnmProcessManager,
                         MPChildKilledComputeUnitsReusedNextRunF,
                         PnmDeviceTypes, print_fmt_test_params);

INSTANTIATE_TEST_SUITE_P(PnmProcessManager, MTWorkerNotReleasedResourceItselfF,
                         PnmDeviceTypes, print_fmt_test_params);

INSTANTIATE_TEST_SUITE_P(PnmProcessManager, MTWorkerNotReleasedResourceKilledF,
                         PnmDeviceTypes, print_fmt_test_params);
