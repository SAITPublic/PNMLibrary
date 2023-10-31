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
#include "core/memory/sls/accessor.h"

#include "sls_suppl.h"

#include "core/device/sls/utils/rank_address.h"
#include "core/memory/imdb/accessor.h"
#include "core/memory/sls/cxl/channel_accessor.h"

#include "common/topology_constants.h"

#include "test/utils/pnm_fmt.h"

#include "pnmlib/core/accessor.h"
#include "pnmlib/core/allocator.h"
#include "pnmlib/core/buffer.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/misc_utils.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest-typed-test.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-type-util.h>

#include <fmt/core.h>
#include <fmt/format.h>

#include <linux/sls_resources.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <vector>

using pnm::sls::device::topo;

TEST(Accessor, WorkWithAllocator) {
  auto context = pnm::make_context(pnm::Device::Type::IMDB);

  static constexpr auto NVALUE = 16;
  auto region = context->allocator()->allocate(NVALUE * sizeof(uint64_t));

  auto acc =
      pnm::memory::Accessor<uint64_t>::create<pnm::memory::ImdbAccessorCore>(
          region, context->device());

  EXPECT_EQ(acc.size(), NVALUE);

  for (auto i = 0; i < NVALUE; ++i) {
    acc[i] = i;
  }

  uint64_t golden_v = 0;
  for (const auto &e : acc) {
    EXPECT_EQ(e, golden_v++);
  }

  fmt::print("{}\n", fmt::join(acc, " "));

  context->allocator()->deallocate(region);
}

class SlsAccessorConstruction : public SlsFixture<uint64_t> {
protected:
  template <typename A> auto print_accessor_vndrange(A &accessor) {
    pnm::memory::VirtualRankedRegion vr =
        std::get<pnm::memory::VirtualRankedRegion>(accessor.virtual_range());
    fmt::print("================================\n");
    for (auto &r : vr) {
      fmt::print("Range: {} - {}\n", static_cast<void *>(r.begin()),
                 static_cast<void *>(r.end()));
    }
  };

  template <typename A> auto check_base(A &accessor) {
    EXPECT_EQ(accessor.size(), VALUES_COUNT);
    EXPECT_NO_THROW(
        std::get<pnm::memory::VirtualRankedRegion>(accessor.virtual_range()));
    EXPECT_NO_THROW(std::get<pnm::memory::RankedRegion>(accessor.phys_range()));
  };

  template <typename A> auto active_range(A &accessor) {
    auto vrange =
        std::get<pnm::memory::VirtualRankedRegion>(accessor.virtual_range());
    return std::count_if(vrange.begin(), vrange.end(),
                         [](auto &r) { return !r.empty(); });
  };
};

TEST_F(SlsAccessorConstruction, Replicate) {
  auto replicate_region = ctx_->allocator()->allocate(
      TEST_DATASET_SIZE,
      pnm::memory::property::AllocPolicy(SLS_ALLOC_REPLICATE_ALL));
  auto accessor_replicate =
      pnm::memory::Accessor<uint64_t>::create<pnm::memory::SlsAccessorCore>(
          replicate_region, ctx_->device());
  check_base(accessor_replicate);
  print_accessor_vndrange(accessor_replicate);
  EXPECT_EQ(active_range(accessor_replicate), topo().NumOfCUnits);

  EXPECT_NO_THROW(ctx_->allocator()->deallocate(replicate_region));
}

TEST_F(SlsAccessorConstruction, Single) {
  auto distribute_region = ctx_->allocator()->allocate(
      TEST_DATASET_SIZE, pnm::memory::property::AllocPolicy(SLS_ALLOC_SINGLE));
  auto accessor_distribute =
      pnm::memory::Accessor<uint64_t>::create<pnm::memory::SlsAccessorCore>(
          distribute_region, ctx_->device());
  check_base(accessor_distribute);
  print_accessor_vndrange(accessor_distribute);
  EXPECT_EQ(active_range(accessor_distribute), 1);

  EXPECT_NO_THROW(ctx_->allocator()->deallocate(distribute_region));
}

TEST_F(SlsAccessorConstruction, FromBuffer) {
  pnm::memory::Buffer<uint64_t> buffer_single(VALUES_COUNT, ctx_);
  auto buffer_accessor_distr = buffer_single.direct_access();
  check_base(buffer_accessor_distr);
  print_accessor_vndrange(buffer_accessor_distr);
  EXPECT_EQ(active_range(buffer_accessor_distr), 1);

  pnm::memory::Buffer<uint64_t> buffer_repl(
      VALUES_COUNT, ctx_,
      pnm::memory::property::AllocPolicy(SLS_ALLOC_REPLICATE_ALL));
  auto buffer_accessor_repl = buffer_repl.direct_access();
  check_base(buffer_accessor_repl);
  print_accessor_vndrange(buffer_accessor_repl);
  EXPECT_EQ(active_range(buffer_accessor_repl), topo().NumOfCUnits);
}

template <typename T> class SlsAccessorOperation : public SlsFixture<T> {
protected:
  void SetUp() override { SlsFixture<T>::SetUp(); }

  void TearDown() override { SlsFixture<T>::TearDown(); }

  void ReadTest(const pnm::memory::RankedRegion &region) {
    for (const auto &r : region.regions) {
      if (r.location.has_value()) {
        auto iptr = pnm::sls::device::InterleavedPointer(
                        this->rank_mem_[*r.location].data(),
                        pnm::sls::device::cunit_to_ha(*r.location)) +
                    r.start;

        std::for_each(this->data_.begin(), this->data_.end(), [&iptr](auto e) {
          *iptr.as<T>() = e;
          iptr += sizeof(T);
        });
      }
    }

    auto accessor = pnm::memory::Accessor<typename SlsFixture<T>::value_type>::
        template create<pnm::memory::SlsAccessorCore>(region,
                                                      this->ctx_->device());
    for (auto i = 0UL; i < this->data_.size(); ++i) {
      ASSERT_EQ(accessor[i], this->data_[i]);
    }

    ASSERT_TRUE(
        std::equal(accessor.begin(), accessor.end(), this->data_.begin()));
  }

  void WriteTest(const pnm::memory::RankedRegion &region) {
    auto accessor = pnm::memory::Accessor<typename SlsFixture<T>::value_type>::
        template create<pnm::memory::SlsAccessorCore>(region,
                                                      this->ctx_->device());

    std::copy(this->data_.begin(), this->data_.end(), accessor.begin());

    for (const auto &r : region.regions) {
      if (r.location.has_value()) {
        auto iptr = pnm::sls::device::InterleavedPointer(
                        this->rank_mem_[*r.location].data(),
                        pnm::sls::device::cunit_to_ha(*r.location)) +
                    r.start;
        for (auto e : this->data_) {
          using vtype = typename SlsFixture<T>::value_type;
          ASSERT_EQ(*iptr.as<vtype>(), e);
          iptr += sizeof(vtype);
        }
      }
    }
  }

  template <typename TestFunction>
  void single_test_run(TestFunction func,
                       pnm::memory::property::AllocPolicy allocation_policy) {
    auto region1 = this->ctx_->allocator()->allocate(this->TEST_DATASET_SIZE,
                                                     allocation_policy);
    func(std::get<pnm::memory::RankedRegion>(region1));

    auto region2 = this->ctx_->allocator()->allocate(this->TEST_DATASET_SIZE,
                                                     allocation_policy);
    func(std::get<pnm::memory::RankedRegion>(region2));

    this->ctx_->allocator()->deallocate(region1);
    this->ctx_->allocator()->deallocate(region2);
  }
};

using TestedTypes =
    ::testing::Types<uint16_t, uint32_t, uint64_t, unsigned __int128>;

TYPED_TEST_SUITE(SlsAccessorOperation, TestedTypes);

TYPED_TEST(SlsAccessorOperation, ReadReplicate) {
  this->single_test_run(
      [this](auto &r) { this->ReadTest(r); },
      pnm::memory::property::AllocPolicy(SLS_ALLOC_REPLICATE_ALL));
}

TYPED_TEST(SlsAccessorOperation, ReadSingle) {
  this->single_test_run([this](auto &r) { this->ReadTest(r); },
                        pnm::memory::property::AllocPolicy(SLS_ALLOC_SINGLE));
}

TYPED_TEST(SlsAccessorOperation, WriteReplicate) {
  this->single_test_run(
      [this](auto &r) { this->WriteTest(r); },
      pnm::memory::property::AllocPolicy(SLS_ALLOC_REPLICATE_ALL));
}

TYPED_TEST(SlsAccessorOperation, WriteSingle) {
  this->single_test_run([this](auto &r) { this->WriteTest(r); },
                        pnm::memory::property::AllocPolicy(SLS_ALLOC_SINGLE));
}

TYPED_TEST(SlsAccessorOperation, MultipleReadWrite) {
  static constexpr auto NALLOC = 10;
  std::vector<pnm::memory::DeviceRegion> mem_alloc(NALLOC);
  for (auto i = 0; i < NALLOC; ++i) {
    mem_alloc[i] = this->ctx_->allocator()->allocate(this->TEST_DATASET_SIZE);
    if (i % 2 == 0) {
      this->WriteTest(std::get<pnm::memory::RankedRegion>(mem_alloc[i]));
    } else {
      this->ReadTest(std::get<pnm::memory::RankedRegion>(mem_alloc[i]));
    }
  }

  std::for_each(mem_alloc.begin(), mem_alloc.end(),
                [this](auto &r) { this->ctx_->allocator()->deallocate(r); });
}

class ChannelAccessorCore : public ::testing::Test {
protected:
  pnm::ContextHandler context_ = pnm::make_context(pnm::Device::Type::SLS);

  using T = uint64_t;

  static constexpr auto elements_count = 1024;
};

TEST_F(ChannelAccessorCore, CreateFromReplicateAll) {
  auto addr = context_->allocator()->allocate(
      elements_count * sizeof(T),
      pnm::memory::property::AllocPolicy(SLS_ALLOC_REPLICATE_ALL));

  auto acc = pnm::memory::Accessor<T>::create<pnm::memory::ChannelAccessorCore>(
      addr, context_->device());

  ASSERT_EQ(acc.size(), elements_count);

  auto virtual_region =
      std::get<pnm::memory::VirtualRankedRegion>(acc.virtual_range());

  static constexpr std::array goldens = {1, 2, 3, 4, 5};

  for (auto vr : virtual_region) {
    ASSERT_NE(vr.begin(), nullptr);
    ASSERT_NE(vr.end(), nullptr);

    auto casted = pnm::views::view_cast<uint64_t>(vr);
    std::copy(goldens.begin(), goldens.end(), casted.begin());
  }

  for (auto i = 0U; i < goldens.size(); ++i) {
    ASSERT_EQ(goldens[i], acc[i]);
  }

  std::iota(acc.begin(), acc.end(), 0);
  for (auto i = 0U; i < acc.size(); ++i) {
    ASSERT_EQ(acc[i], i);

    for (auto vr : virtual_region) {
      auto casted = pnm::views::view_cast<uint64_t>(vr);
      ASSERT_EQ(casted[i], i);
    }
  }

  context_->allocator()->deallocate(addr);
}

TEST_F(ChannelAccessorCore, CreateFromSingleChannel) {
  for (auto cunit = 0U; cunit < topo().NumOfCUnits; ++cunit) {
    auto addr = context_->allocator()->allocate(
        elements_count * sizeof(T), pnm::memory::property::CURegion(cunit));

    auto acc =
        pnm::memory::Accessor<T>::create<pnm::memory::ChannelAccessorCore>(
            addr, context_->device());

    ASSERT_EQ(acc.size(), elements_count);

    auto virtual_region =
        std::get<pnm::memory::VirtualRankedRegion>(acc.virtual_range());

    auto predicate = [](pnm::views::common<uint8_t> vr) {
      return vr.begin() != nullptr && vr.end() != nullptr;
    };

    ASSERT_TRUE(pnm::utils::one_of(virtual_region.begin(), virtual_region.end(),
                                   predicate));

    auto active_vr =
        std::find_if(virtual_region.begin(), virtual_region.end(), predicate);

    static constexpr std::array goldens = {1, 2, 3, 4, 5};
    auto casted = pnm::views::view_cast<uint64_t>(*active_vr);
    std::copy(goldens.begin(), goldens.end(), casted.begin());

    for (auto i = 0U; i < goldens.size(); ++i) {
      ASSERT_EQ(goldens[i], acc[i]);
    }

    std::iota(acc.begin(), acc.end(), 0);
    for (auto i = 0U; i < acc.size(); ++i) {
      ASSERT_EQ(acc[i], i);

      ASSERT_EQ(casted[i], i);
    }

    context_->allocator()->deallocate(addr);
  }
}
