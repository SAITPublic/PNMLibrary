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

#include "sls_suppl.h"

#include "core/device/sls/base.h"
#include "core/memory/imdb/accessor.h"
#include "core/memory/sls/accessor.h"
#include "core/memory/sls/cxl/channel_accessor.h"

#include "common/make_error.h"
#include "common/topology_constants.h"

#include "pnmlib/core/accessor.h"
#include "pnmlib/core/allocator.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest-typed-test.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-type-util.h>

#include <linux/sls_resources.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <vector>

struct TransferManager : ::testing::Test {
protected:
  void SetUp() override {
    ctx = pnm::make_context(pnm::Device::Type::IMDB);
    region = ctx->allocator()->allocate(bytes);
  }
  void TearDown() override { ctx->allocator()->deallocate(region); }

  auto get_accessor() {
    return pnm::memory::Accessor<uint32_t>::create<
        pnm::memory::ImdbAccessorCore>(region, ctx->device());
  };

  static constexpr auto N = 256;
  static constexpr auto bytes = N * sizeof(uint32_t);
  pnm::memory::DeviceRegion region;
  pnm::ContextHandler ctx;
};

TEST_F(TransferManager, Forward) {
  std::vector<uint32_t> big(N * 2);
  std::vector<uint32_t> small(N / 2);
  std::vector<uint32_t> equal(N);

  auto fill = [](auto &v) { std::iota(v.begin(), v.end(), 1); };
  fill(big);
  fill(small);
  fill(equal);

  auto acc = get_accessor();
  std::fill(acc.begin(), acc.end(), 0);

  EXPECT_THROW(ctx->transfer_manager()->copy_to_device(
                   pnm::views::make_view(big), get_accessor()),
               pnm::error::InvalidArguments);

  uint64_t transfered_size;
  EXPECT_NO_THROW(transfered_size = ctx->transfer_manager()->copy_to_device(
                      pnm::views::make_view(small), get_accessor()));
  EXPECT_EQ(transfered_size, small.size());
  EXPECT_TRUE(std::equal(small.begin(), small.end(), acc.begin()));

  auto zeroes_start = acc.begin();
  zeroes_start += small.size();
  EXPECT_TRUE(
      std::all_of(zeroes_start, acc.end(), [](auto e) { return e == 0; }));

  std::fill(acc.begin(), acc.end(), 0);
  EXPECT_NO_THROW(transfered_size = ctx->transfer_manager()->copy_to_device(
                      pnm::views::make_view(equal), get_accessor()));
  EXPECT_EQ(transfered_size, equal.size());
  EXPECT_TRUE(std::equal(equal.begin(), equal.end(), acc.begin()));
}

TEST_F(TransferManager, Backward) {
  std::vector<uint32_t> big(N * 2, 0);
  std::vector<uint32_t> small(N / 2, 0);
  std::vector<uint32_t> equal(N, 0);

  auto acc = get_accessor();
  std::iota(acc.begin(), acc.end(), 42);

  EXPECT_THROW(ctx->transfer_manager()->copy_from_device(
                   get_accessor(), pnm::views::make_view(small)),
               pnm::error::InvalidArguments);

  uint64_t transfered_size;
  EXPECT_NO_THROW(transfered_size = ctx->transfer_manager()->copy_from_device(
                      get_accessor(), pnm::views::make_view(big)));
  EXPECT_EQ(transfered_size, acc.size());
  EXPECT_TRUE(std::equal(acc.begin(), acc.end(), big.begin()));

  auto zeroes_start = big.begin() + acc.size();
  EXPECT_TRUE(
      std::all_of(zeroes_start, big.end(), [](auto &e) { return e == 0; }));

  EXPECT_NO_THROW(transfered_size = ctx->transfer_manager()->copy_from_device(
                      get_accessor(), pnm::views::make_view(equal)));
  EXPECT_EQ(transfered_size, equal.size());
  EXPECT_TRUE(std::equal(acc.begin(), acc.end(), equal.begin()));
}

template <typename T> struct SlsTransferManager : public SlsFixture<T> {
protected:
  auto make_accessor(const pnm::memory::DeviceRegion &region) {
    switch (pnm::sls::device::topo().Bus) {

    case pnm::sls::device::BusType::AXDIMM:
      return pnm::memory::Accessor<typename SlsFixture<T>::value_type>::
          template create<pnm::memory::SlsAccessorCore>(region,
                                                        this->ctx_->device());
    case pnm::sls::device::BusType::CXL:
      return pnm::memory::Accessor<typename SlsFixture<T>::value_type>::
          template create<pnm::memory::ChannelAccessorCore>(
              region, this->ctx_->device());
    }

    throw pnm::error::make_inval("SLS device type");
  }

  auto allocate(pnm::memory::property::AllocPolicy policy) {
    return this->ctx_->allocator()->allocate(this->TEST_DATASET_SIZE, policy);
  }

  auto deallocate(const pnm::memory::DeviceRegion &region) {
    ASSERT_NO_THROW(this->ctx_->allocator()->deallocate(region));
  }

  void round_transfer_check(const pnm::memory::DeviceRegion &addr) {
    auto count = this->ctx_->transfer_manager()->copy_to_device(
        pnm::views::make_view(this->data_), this->make_accessor(addr));
    ASSERT_EQ(count, this->data_.size());

    auto probe_accessor = this->make_accessor(addr);
    ASSERT_TRUE(std::equal(this->data_.begin(), this->data_.end(),
                           probe_accessor.begin()));

    std::vector<typename decltype(this->data_)::value_type> out(
        this->data_.size());

    count = this->ctx_->transfer_manager()->copy_from_device(
        std::move(probe_accessor), pnm::views::make_view(out));
    ASSERT_EQ(count, out.size());

    ASSERT_EQ(out, this->data_);
  }
};

using TestedTypes =
    ::testing::Types<uint16_t, uint32_t, uint64_t, unsigned __int128>;

TYPED_TEST_SUITE(SlsTransferManager, TestedTypes);

TYPED_TEST(SlsTransferManager, Distribute) {
  pnm::memory::DeviceRegion addr;

  ASSERT_NO_THROW(addr = this->allocate(
                      pnm::memory::property::AllocPolicy(SLS_ALLOC_SINGLE)));

  this->round_transfer_check(addr);

  this->deallocate(addr);
}

TYPED_TEST(SlsTransferManager, Replicate) {
  pnm::memory::DeviceRegion addr;

  ASSERT_NO_THROW(addr = this->allocate(pnm::memory::property::AllocPolicy(
                      SLS_ALLOC_REPLICATE_ALL)));

  this->round_transfer_check(addr);

  for (auto &r : std::get<pnm::memory::RankedRegion>(addr).regions) {
    ASSERT_TRUE(r.location.has_value());

    const auto cunit = *r.location;

    const pnm::sls::device::BaseDevice *dev =
        this->ctx_->device()->template as<pnm::sls::device::BaseDevice>();

    uintptr_t offset = 0;
    for (auto e : this->data_) {
      using vtype = typename decltype(this->data_)::value_type;
      vtype val;

      const auto *ptr = dev->mem_block_handler()->get_mem_block_ptr(
          SLS_BLOCK_BASE, cunit, offset);

      std::memcpy(&val, ptr, sizeof(val));

      ASSERT_EQ(val, e);

      offset += sizeof(vtype);
    }
  }

  this->deallocate(addr);
}
