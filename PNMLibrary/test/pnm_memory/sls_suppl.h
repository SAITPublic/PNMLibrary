/*
 *  Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 *  This software is proprietary of Samsung Electronics.
 *  No part of this software, either material or conceptual may be copied or
 *  distributed, transmitted, transcribed, stored in a retrieval system or
 *  translated into any human or computer language in any form by any means,
 *  electronic, mechanical, manual or otherwise, or disclosed to third parties
 *  without the express written permission of Samsung Electronics.
 */

#ifndef PNM_SLS_SUPPL_H
#define PNM_SLS_SUPPL_H

#include "common/file_descriptor_holder.h"
#include "common/topology_constants.h"

#include "pnmlib/sls/control.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"

#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <linux/sls_resources.h>

#include <fcntl.h>
#include <sys/mman.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

using pnm::sls::device::topo;

template <typename T> class SlsFixture : public ::testing::Test {
protected:
  using value_type = T;

  SlsFixture()
      : rank_mem_(topo().NumOfCUnits),
        device_(pnm::sls::device::get_device_path(), O_RDWR) {}

  void SetUp() override {
    const pnm::sls::device::Control ctrl_context;
    const auto mem_info = ctrl_context.get_mem_info();

    for (auto rid = 0U; rid < topo().NumOfCUnits; ++rid) {
      const auto &block_info = mem_info[rid][SLS_BLOCK_BASE];
      auto *memory = static_cast<uint8_t *>(
          mmap(nullptr, block_info.map_size, PROT_READ | PROT_WRITE, MAP_SHARED,
               *device_, block_info.map_offset));
      ASSERT_NE(memory, MAP_FAILED);

      auto size = mem_info[rid][SLS_BLOCK_BASE].size;

      rank_mem_[rid] = pnm::views::make_view(memory, size);
    }
    ctx_ = pnm::make_context(pnm::Device::Type::SLS);

    data_.resize(VALUES_COUNT);

    value_type iter = 0;
    auto generator = [&iter]() {
      static constexpr auto max_value = std::numeric_limits<value_type>::max();
      iter = (iter + 1) % max_value;
      return iter;
    };
    std::generate(data_.begin(), data_.end(), generator);
  }

  void TearDown() override {
    for (auto rid = 0U; rid < topo().NumOfCUnits; ++rid) {
      auto *addr = rank_mem_[rid].data();
      auto size = rank_mem_[rid].size();

      ASSERT_EQ(munmap(addr, size), 0);
    }
  }

  std::vector<pnm::views::common<uint8_t>> rank_mem_;
  pnm::utils::FileDescriptorHolder device_;
  pnm::ContextHandler ctx_;

  static constexpr auto TEST_DATASET_SIZE = 8 << 20; // MB
  static constexpr auto VALUES_COUNT = TEST_DATASET_SIZE / sizeof(value_type);
  std::vector<T> data_;
};

#endif // PNM_SLS_SUPPL_H
