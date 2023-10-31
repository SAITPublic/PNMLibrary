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
#include "pnmlib/core/allocator.h"

#include "core/device/imdb/base.h"

#include "common/topology_constants.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/error.h"

#include "gtest/gtest.h"

#include <linux/sls_resources.h>

#include <algorithm>
#include <utility>
#include <vector>

using pnm::sls::device::topo;

class SequentialAllocator : public ::testing::Test {
protected:
  void SetUp() override { ctx_ = pnm::make_context(pnm::Device::Type::IMDB); }

  auto &alloc() { return ctx_->allocator(); }

  auto &get(pnm::memory::DeviceRegion &region) {
    return std::get<pnm::memory::SequentialRegion>(region);
  }

  auto alignment() {
    auto *dev = ctx_->device().get();
    auto alignment = dev->as<pnm::imdb::device::BaseDevice>()->alignment();
    return alignment;
  }

private:
  pnm::ContextHandler ctx_;
};

TEST_F(SequentialAllocator, SizeAndAlign) {
  pnm::memory::DeviceRegion one;
  ASSERT_NO_THROW(one = this->alloc()->allocate(1));
  EXPECT_NO_THROW(std::get<pnm::memory::SequentialRegion>(one));
  EXPECT_EQ(get(one).size, 1);
  EXPECT_EQ(get(one).start % this->alignment(), 0);

  auto size = 142;
  pnm::memory::DeviceRegion medium;
  ASSERT_NO_THROW(medium = this->alloc()->allocate(size));
  EXPECT_EQ(get(medium).size, 142);
  EXPECT_EQ(get(medium).start % this->alignment(), 0);

  alloc()->deallocate(one);
  alloc()->deallocate(medium);

  for (auto i = 1; i < 8192; ++i) {
    pnm::memory::DeviceRegion dregion;
    ASSERT_NO_THROW(dregion = alloc()->allocate(i));
    EXPECT_EQ(get(dregion).size, i);
    EXPECT_EQ(get(dregion).start % this->alignment(), 0);
    alloc()->deallocate(dregion);
  }
}

TEST(SlsAllocator, AllocationFail) {
  auto ctx = pnm::make_context(pnm::Device::Type::SLS);
  auto &alloc = ctx->allocator();

  EXPECT_THROW(alloc->allocate(0), pnm::error::InvalidArguments);
  EXPECT_THROW(alloc->allocate(1UL << 60), pnm::error::OutOfMemory);
}

TEST(SlsAllocator, AllocateOnes) {
  auto ctx = pnm::make_context(pnm::Device::Type::SLS);
  auto &alloc = ctx->allocator();

  auto get = [](auto &variant) -> pnm::memory::RankedRegion & {
    return std::get<pnm::memory::RankedRegion>(variant);
  };

  // Test allocator for region size 1 and 142
  for (auto alloc_size : {1, 142}) {
    pnm::memory::DeviceRegion one;
    ASSERT_NO_THROW(
        one = alloc->allocate(alloc_size, pnm::memory::property::AllocPolicy{
                                              SLS_ALLOC_REPLICATE_ALL}));
    EXPECT_NO_THROW(std::get<pnm::memory::RankedRegion>(one));
    EXPECT_EQ(get(one).regions.size(), topo().NumOfCUnits);

    for (auto i = 0U; i < topo().NumOfCUnits; ++i) {
      EXPECT_EQ(get(one).regions[i].size, alloc_size);
    }

    ASSERT_NO_THROW(alloc->deallocate(one));

    ASSERT_NO_THROW(one = alloc->allocate(alloc_size));
    EXPECT_NO_THROW(std::get<pnm::memory::RankedRegion>(one));
    EXPECT_EQ(get(one).regions.size(), topo().NumOfCUnits);

    EXPECT_EQ(
        std::count_if(get(one).regions.begin(), get(one).regions.end(),
                      [](const auto &e) { return !e.location.has_value(); }),
        topo().NumOfCUnits - 1);

    for (auto &r : get(one).regions) {
      if (r.location.has_value()) {
        EXPECT_EQ(r.size, alloc_size);
      }
    }

    ASSERT_NO_THROW(alloc->deallocate(one));
  }
}

TEST(SlsAllocator, AllocateSequence) {
  auto ctx = pnm::make_context(pnm::Device::Type::SLS);
  auto &alloc = ctx->allocator();

  auto get = [](auto &variant) {
    return std::get<pnm::memory::RankedRegion>(variant);
  };

  static constexpr auto ALLOCATION_ATTEMPTS = 8192;
  for (auto i = 1; i <= ALLOCATION_ATTEMPTS; ++i) {
    pnm::memory::DeviceRegion dregion;
    ASSERT_NO_THROW(dregion = alloc->allocate(i));
    const auto &rref = get(dregion);
    EXPECT_EQ(rref.regions.size(), topo().NumOfCUnits);
    EXPECT_NO_THROW(alloc->deallocate(dregion));
  }

  for (auto i = 1; i <= ALLOCATION_ATTEMPTS; ++i) {
    pnm::memory::DeviceRegion dregion;
    ASSERT_NO_THROW(
        dregion = alloc->allocate(
            i, pnm::memory::property::AllocPolicy{SLS_ALLOC_REPLICATE_ALL}));
    const auto &rref = get(dregion);
    EXPECT_EQ(rref.regions.size(), topo().NumOfCUnits);
    EXPECT_NO_THROW(alloc->deallocate(dregion));
  }
}

TEST(SlsAllocator, DeferredDeallocation) {
  auto ctx = pnm::make_context(pnm::Device::Type::SLS);
  auto &alloc = ctx->allocator();

  auto get = [](auto &variant) {
    return std::get<pnm::memory::RankedRegion>(variant);
  };

  std::vector<pnm::memory::DeviceRegion> regions;

  static constexpr auto ALLOCATION_ATTEMPTS = 8192;
  for (auto i = 1; i <= ALLOCATION_ATTEMPTS; ++i) {
    pnm::memory::DeviceRegion dregion;
    ASSERT_NO_THROW(dregion = alloc->allocate(i));
    const auto &rref = get(dregion);
    EXPECT_EQ(rref.regions.size(), topo().NumOfCUnits);
    regions.emplace_back(std::move(dregion));
  }

  ASSERT_NO_THROW(
      std::for_each(regions.begin(), regions.end(), [&alloc](auto &r) {
        EXPECT_NO_THROW(alloc->deallocate(r));
      }));
}
