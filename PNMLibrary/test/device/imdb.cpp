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

#include "core/device/imdb/base.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/error.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <future>
#include <random>
#include <vector>

class ImdbAllocatorFixture : public ::testing::Test {
protected:
  pnm::ContextHandler ctx;
  pnm::imdb::device::BaseDevice *dev;

  void SetUp() override {
    ctx = pnm::make_context(pnm::Device::Type::IMDB);
    dev = ctx->device()->as<pnm::imdb::device::BaseDevice>();
  }
};

TEST_F(ImdbAllocatorFixture, AllocatorBasic) {
  const auto mem_size = dev->memory_size();
  const auto free_size = dev->free_size();
  const auto alignment = dev->alignment();

  EXPECT_EQ(mem_size, free_size);

  pnm::memory::DeviceRegion region{};

  EXPECT_NO_THROW(region = ctx->allocator()->allocate(alignment / 2));

  EXPECT_EQ(dev->free_size(), free_size - alignment);

  EXPECT_NO_THROW(ctx->allocator()->deallocate(region));

  EXPECT_EQ(dev->free_size(), free_size);
}

TEST_F(ImdbAllocatorFixture, AllocDealloc) {
  // random constants
  static constexpr int alloc_cnt = 42;
  const long long alloc_size = dev->alignment() * 42;

  const long long mem_size = dev->memory_size();
  std::vector<pnm::memory::DeviceRegion> regions;
  regions.reserve(alloc_cnt);

  for (int i = 1; i <= alloc_cnt; i++) {
    EXPECT_NO_THROW(regions.push_back(ctx->allocator()->allocate(alloc_size)));
    EXPECT_EQ(dev->free_size(), mem_size - i * alloc_size);
  }

  std::shuffle(regions.begin(), regions.end(),
               std::mt19937{std::random_device{}()});

  for (int i = alloc_cnt - 1; i >= 0; i--) {
    EXPECT_NO_THROW(ctx->allocator()->deallocate(regions.back()));
    regions.pop_back();
    EXPECT_EQ(dev->free_size(), mem_size - i * alloc_size);
  }

  EXPECT_EQ(dev->free_size(), dev->memory_size());
}

TEST_F(ImdbAllocatorFixture, AllocateEverything) {
  const long long alloc_size = dev->alignment() * 4;
  const int alloc_cnt = dev->memory_size() / alloc_size;

  const long long mem_size = dev->memory_size();
  std::vector<pnm::memory::DeviceRegion> regions;
  regions.reserve(alloc_cnt);

  for (int i = 1; i <= alloc_cnt; i++) {
    EXPECT_NO_THROW(regions.push_back(ctx->allocator()->allocate(alloc_size)));
    EXPECT_EQ(dev->free_size(), mem_size - i * alloc_size);
  }

  EXPECT_THROW(ctx->allocator()->allocate(alloc_size), pnm::error::OutOfMemory);

  std::shuffle(regions.begin(), regions.end(),
               std::mt19937{std::random_device{}()});

  for (int i = alloc_cnt - 1; i >= 0; i--) {
    EXPECT_NO_THROW(ctx->allocator()->deallocate(regions.back()));
    regions.pop_back();
    EXPECT_EQ(dev->free_size(), mem_size - i * alloc_size);
  }

  EXPECT_EQ(dev->free_size(), dev->memory_size());
}

TEST_F(ImdbAllocatorFixture, AllocDeallocMT) {
  auto job = [this]() {
    // currently we can allocate up to 34359738368 / 2097152 = 16384
    // alignments, so make sure that we won't allocate more
    // (alloc_cnt * alloc_size * workers_cnt < 16384)
    static constexpr int alloc_cnt = 73;
    const long long alloc_size = dev->alignment() * 37;

    std::vector<pnm::memory::DeviceRegion> regions;
    regions.reserve(alloc_cnt);

    for (int i = 1; i <= alloc_cnt; i++) {
      regions.push_back(ctx->allocator()->allocate(alloc_size));
    }

    std::shuffle(regions.begin(), regions.end(),
                 std::mt19937{std::random_device{}()});

    for (int i = alloc_cnt - 1; i >= 0; i--) {
      ctx->allocator()->deallocate(regions.back());
      regions.pop_back();
    }
  };

  static constexpr auto workers_cnt = 4;
  for (int j = 0; j < 100; j++) {
    std::vector<std::future<void>> workers;
    workers.reserve(workers_cnt);
    for (int i = 0; i < workers_cnt; i++) {
      workers.push_back(std::async(std::launch::async, job));
    }

    for (auto &worker : workers) {
      EXPECT_NO_THROW(worker.get());
    }

    EXPECT_EQ(dev->free_size(), dev->memory_size());
  }
}
