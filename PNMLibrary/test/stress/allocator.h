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

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "common/topology_constants.h"

#include "test/utils/pnm_fmt.h"

#include "pnmlib/core/allocator.h"
#include "pnmlib/core/buffer.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"

#include "pnmlib/common/error.h"

#include <gtest/gtest.h>
#include <gtest/internal/gtest-param-util.h>

#include <fmt/core.h>

#include <linux/sls_resources.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <random>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <vector>

using pnm::sls::device::topo;

inline constexpr size_t allocations_count = 100'000;
inline constexpr size_t threads_count = 20;
/*
 * [TODO: @s.motov MCS-324 MCS-325] add tests for
 * DISTRIBUTE_EVEN and DISTRIBUTE_LU policies
 */
inline constexpr std::array alloc_pref{SLS_ALLOC_SINGLE,
                                       SLS_ALLOC_REPLICATE_ALL};

//! @brief modes test how driver react to memory overflow
enum class StressMode : uint8_t {
  //! @brief fill SLS memory and test return error
  capacity = 0,

  //! @brief fill SLS memory and test return error
  //! free sls memory
  //! fill SLS memory again and compare first and second allocated sizes
  capacityDouble,

  //! @brief fill SLS memory and test return error
  //! free random 3/4 of memory
  //! fill SLS memory again and compare first and second allocated sizes
  capacityFragmentation,

  //! @ brief fill SLS memory in multithread
  multithreadedCapacity
};

constexpr auto format_as(StressMode mode) {
  switch (mode) {
  case StressMode::capacity:
    return "CAPACITY";
  case StressMode::capacityDouble:
    return "CAPACITY_DOUBLE";
  case StressMode::capacityFragmentation:
    return "CAPACITY_FRAGMENTATION";
  case StressMode::multithreadedCapacity:
    return "MULTITHREADED";
  }
  throw std::runtime_error(
      fmt::format("Unknown test mode {}", static_cast<int>(mode)));
}

struct RunArgs {
  StressMode stress_mode;
  sls_user_preferences preference;

  void print() const {
    fmt::print("STRESS MODE: {}\n", stress_mode);
    fmt::print("DISTRIBUTE PREFERENCE: {}\n", preference);
  }
};

//! @brief This class simplify work with allocator
// - count allocated size
// - keep allocated descriptors and delete allocations in dtor
// - can randomly deallocate memory
class AllocHelper {
public:
  explicit AllocHelper(sls_user_preferences pref) : pref_(pref) {
    alloc_request_size_ = expected_size() / allocations_count;

    allocations_.reserve(allocations_count);
  }

  ~AllocHelper() {
    fmt::print("ALLOCATIONS COUNT: {}\n", allocations_.size());
    random_clear(1);
    EXPECT_EQ(allocated_size_, 0);
  }

  [[nodiscard]] auto get_preference() const { return pref_; }

  //! @brief allocate memory
  void allocate() {
    allocations_.emplace_back(alloc_request_size_, context,
                              pnm::memory::property::AllocPolicy(pref_));

    allocated_size_ += alloc_request_size_;
  }

  //! @brief get allocated size
  [[nodiscard]] auto allocated_size() const { return allocated_size_; }

  //! @brief get size of one allocation
  [[nodiscard]] auto allocation_request_size() const {
    return alloc_request_size_;
  }

  //! @brief get sls size to alloc
  [[nodiscard]] uint64_t expected_size() const {
    const uint64_t total_memory_size = context->device()->memory_size();

    switch (pref_) {
    case SLS_ALLOC_AUTO:
    case SLS_ALLOC_REPLICATE_ALL:
      return total_memory_size / topo().NumOfCUnits;
    case SLS_ALLOC_DISTRIBUTE_ALL:
      return total_memory_size;
    case SLS_ALLOC_SINGLE:
      return total_memory_size;
    }

    throw std::runtime_error(fmt::format("Unknown pref {}", pref_));
  }

  //! @brief randomly deallocate memory according to value [0.0, 1.0]
  void random_clear(double value) {
    std::default_random_engine gen{std::random_device{}()};
    std::bernoulli_distribution distrib(value);

    auto deleter = [&](pnm::memory::Buffer<uint8_t> &alloc) {
      const auto does_remove = distrib(gen);

      if (!does_remove) {
        return false;
      }

      allocated_size_ -= alloc.size();

      return true;
    };

    allocations_.erase(
        std::remove_if(allocations_.begin(), allocations_.end(), deleter),
        allocations_.end());
  }

private:
  const pnm::ContextHandler context = pnm::make_context(pnm::Device::Type::SLS);

  const sls_user_preferences pref_;
  std::vector<pnm::memory::Buffer<uint8_t>> allocations_;

  uint64_t allocated_size_ = 0;
  size_t alloc_request_size_;
};

inline void capacity_test(AllocHelper &gen) {
  try {
    while (true) {
      gen.allocate();
    }
  } catch (pnm::error::OutOfMemory &) {
    // ok
  } catch (std::exception &e) {
    FAIL() << "Got incorrect exception: " << e.what();
  }
}

inline void capacity_double_test(AllocHelper &gen) {
  capacity_test(gen);

  const auto allocated_size = gen.allocated_size();

  gen.random_clear(1);

  capacity_test(gen);

  EXPECT_EQ(allocated_size, gen.allocated_size());
}

inline void capacity_fragmentation_test(AllocHelper &gen) {
  capacity_test(gen);

  const auto size = gen.allocated_size();

  gen.random_clear(0.75);

  capacity_test(gen);

  EXPECT_EQ(size, gen.allocated_size());
}

inline void multithreaded_capacity_test(AllocHelper &gen) {
  std::vector<std::thread> workers(threads_count);
  std::atomic<size_t> barrier(threads_count);

  for (auto &worker : workers) {
    worker = std::thread([&]() {
      AllocHelper thread_gen(gen.get_preference());

      /* wait all thread initialized */
      --barrier;
      while (barrier != 0) {
        ;
      };

      try {
        while (true) {
          thread_gen.allocate();
        }
      } catch (pnm::error::OutOfMemory &) {
        // ok
      } catch (std::exception &e) {
        FAIL() << "Got incorrect exception: " << e.what();
      }
    });
  }

  for (auto &worker : workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

inline std::function<void(AllocHelper &)> get_executor(StressMode mode) {
  switch (mode) {
  case StressMode::capacity:
    return capacity_test;
  case StressMode::capacityDouble:
    return capacity_double_test;
  case StressMode::capacityFragmentation:
    return capacity_fragmentation_test;
  case StressMode::multithreadedCapacity:
    return multithreaded_capacity_test;
  }
  throw std::runtime_error(
      fmt::format("Unknown executor {}", static_cast<size_t>(mode)));
}

class UnitTest {
public:
  explicit UnitTest(const RunArgs &args)
      : alloc_helper_(args.preference),
        test_func_(get_executor(args.stress_mode)) {
    args.print();
  }

  void run() {
    test_func_(alloc_helper_);

    const auto expected_size = alloc_helper_.expected_size();
    const auto allocated_size = alloc_helper_.allocated_size();
    const auto difference = expected_size - allocated_size;

    fmt::print("EXPECTED SIZE: {}\n", expected_size);
    fmt::print("ALLOCATED SIZE: {}\n", allocated_size);
    fmt::print("DIFFERENCE: {}\n", difference);
    fmt::print("ALLOCATION SIZE {}\n", alloc_helper_.allocation_request_size());
    fmt::print("ALLOCATION FACTOR {}\n",
               difference / alloc_helper_.allocation_request_size());
  }

private:
  AllocHelper alloc_helper_;
  std::function<void(AllocHelper &)> test_func_;
};

class Allocator : public testing::TestWithParam<
                      std::tuple<StressMode, sls_user_preferences>> {};

inline auto
print_test_name(const testing::TestParamInfo<Allocator::ParamType> &info) {
  const auto [mode, preference] = info.param;

  return fmt::format("STRESS_MODE_{}__PREFERENCE_{}", mode, preference);
};

#endif // ALLOCATOR_H
