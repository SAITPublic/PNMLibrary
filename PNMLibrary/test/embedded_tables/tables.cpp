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

#include "sls/operation/internal.h"
#include "sls/operation/reduction_operation.h"

#include "core/device/sls/rank_memory.h"

#include "common/topology_constants.h"

#include "pnmlib/sls/embedded_tables.h"
#include "pnmlib/sls/operation.h"
#include "pnmlib/sls/type.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"
#include "pnmlib/core/sls_device.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <fmt/core.h>
#include <fmt/format.h>

#include <linux/sls_resources.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class EmbeddedTablesCore : public ::testing::TestWithParam<pnm::Device::Type> {
protected:
  void SetUp() override { context = pnm::make_context(GetParam()); }

  static constexpr uint64_t num_user_objects = 5;
  static constexpr uint32_t user_object_length = 100'000'000;

  std::vector<uint32_t> lengths =
      std::vector<uint32_t>(num_user_objects, user_object_length);
  static constexpr uint32_t sparse_feature_bytes = sizeof(uint8_t);

  const auto &sls_range(const pnm::memory::DeviceRegion &region) {
    return std::get<pnm::memory::RankedRegion>(region);
  }

  pnm::ContextHandler context;
};

INSTANTIATE_TEST_SUITE_P(EmbeddedTable, EmbeddedTablesCore,
                         ::testing::Values(pnm::Device::Type::SLS_AXDIMM));

TEST_P(EmbeddedTablesCore, ReplicateAll) {
  pnm::memory::EmbeddedTablesHandler pack;

  EXPECT_NO_THROW(
      pack = pnm::memory::EmbeddedTables::create(
          lengths, sparse_feature_bytes, context, SLS_ALLOC_REPLICATE_ALL));

  const auto &mem_objects = pack->buffers();

  const auto address_dim = (GetParam() == pnm::Device::Type::SLS_AXDIMM)
                               ? pnm::device::topo().NumOfRanks
                               : pnm::device::topo().NumOfChannels;

  for (uint8_t i = 0; i < address_dim; ++i) {
    fmt::print("Address block [{:d}]\n", i);
    for (size_t object_idx = 0; object_idx < num_user_objects; ++object_idx) {
      const uint8_t rank_num =
          sls_range(mem_objects[object_idx].device_region())
              .regions[i]
              .location;

      fmt::print("table [{}] is on rank [{:d}]\n", object_idx, rank_num);

      const uint64_t length =
          sls_range(mem_objects[object_idx].device_region()).regions[i].size;
      fmt::print("length [{}]\n", length);
      EXPECT_EQ(length, lengths[object_idx]);
    }
  }
}

TEST_P(EmbeddedTablesCore, CompareAllocations) {
  auto dev = SlsDevice::make(GetParam());
  const auto total_size = dev.base_memory_size();

  const auto rank_size = total_size / pnm::device::topo().NumOfRanks;
  const auto half_rank_size = rank_size / 2;
  const auto ext_num_user_objects = half_rank_size / user_object_length;
  const std::vector<uint32_t> ext_lengths(ext_num_user_objects,
                                          user_object_length);

  pnm::memory::EmbeddedTablesHandler pack1, pack2;

  EXPECT_NO_THROW(
      pack1 = pnm::memory::EmbeddedTables::create(
          ext_lengths, sparse_feature_bytes, context, SLS_ALLOC_REPLICATE_ALL));

  EXPECT_NO_THROW(
      pack2 = pnm::memory::EmbeddedTables::create(
          ext_lengths, sparse_feature_bytes, context, SLS_ALLOC_REPLICATE_ALL));

  for (size_t i = 0; i < pack1->buffers().size(); ++i) {
    EXPECT_FALSE(pack1->buffers()[i].device_region() ==
                 pack2->buffers()[i].device_region());
  }
}

TEST_P(EmbeddedTablesCore, ExtensiveReplicateAll) {
  auto dev = SlsDevice::make(GetParam());
  const auto total_size = dev.base_memory_size();
  fmt::print("Total ranks size: {}\n", total_size);

  const auto rank_size = total_size / pnm::device::topo().NumOfRanks;
  fmt::print("Rank size: {}\n", rank_size);

  const auto three_fourth_rank = rank_size / 4 * 3;
  fmt::print("Three fourth of rank size: {}\n", three_fourth_rank);

  const auto ext_num_user_objects = three_fourth_rank / user_object_length;

  const std::vector<uint32_t> ext_lengths(ext_num_user_objects,
                                          user_object_length);

  pnm::memory::EmbeddedTablesHandler pack, pack_fail;

  EXPECT_NO_THROW(
      pack = pnm::memory::EmbeddedTables::create(
          ext_lengths, sparse_feature_bytes, context, SLS_ALLOC_REPLICATE_ALL));

  EXPECT_THROW(
      pack_fail = pnm::memory::EmbeddedTables::create(
          ext_lengths, sparse_feature_bytes, context, SLS_ALLOC_REPLICATE_ALL),
      pnm::error::OutOfMemory);

  pack.reset(nullptr);
  EXPECT_NO_THROW(
      pack = pnm::memory::EmbeddedTables::create(
          ext_lengths, sparse_feature_bytes, context, SLS_ALLOC_REPLICATE_ALL));
}

TEST_P(EmbeddedTablesCore, ExtensiveDistributeAll) {
  const auto dev = SlsDevice::make(GetParam());

  const auto total_size = dev.base_memory_size();
  fmt::print("Total ranks size: {}\n", total_size);

  const auto three_fourth = total_size / pnm::device::topo().NumOfRanks * 3;
  fmt::print("Three fourth of all ranks: {}\n", three_fourth);

  const auto ext_num_user_objects = three_fourth / user_object_length;

  const std::vector<uint32_t> ext_lengths(ext_num_user_objects,
                                          user_object_length);

  pnm::memory::EmbeddedTablesHandler pack, pack_fail;

  EXPECT_NO_THROW(pack = pnm::memory::EmbeddedTables::create(
                      ext_lengths, sparse_feature_bytes, context,
                      SLS_ALLOC_DISTRIBUTE_ALL));

  EXPECT_THROW(
      pack_fail = pnm::memory::EmbeddedTables::create(
          ext_lengths, sparse_feature_bytes, context, SLS_ALLOC_DISTRIBUTE_ALL),
      pnm::error::OutOfMemory);

  pack.reset(nullptr);
  EXPECT_NO_THROW(pack = pnm::memory::EmbeddedTables::create(
                      ext_lengths, sparse_feature_bytes, context,
                      SLS_ALLOC_DISTRIBUTE_ALL));
}

TEST_P(EmbeddedTablesCore, TrivialInstructionGeneration) {
  pnm::memory::EmbeddedTablesHandler etable;

  static constexpr uint64_t extra_num_user_objects = 5;
  static constexpr uint32_t extra_user_object_length = 100'000;

  const std::vector<uint32_t> extra_lengths =
      std::vector<uint32_t>(extra_num_user_objects, extra_user_object_length);
  static constexpr uint32_t sparse_feature_size = 16;

  EXPECT_NO_THROW(etable = pnm::memory::EmbeddedTables::create(
                      extra_lengths, sparse_feature_size * sizeof(uint32_t),
                      context, SLS_ALLOC_DISTRIBUTE_ALL));

  PNMSLSOperation op(sparse_feature_size, pnm::make_view(extra_lengths),
                     etable.get(), SLSType::Uint32);
  static constexpr auto minibatch_size = 5;
  static constexpr auto lookup_count = 10;
  const std::vector<uint32_t> lookups_size(
      minibatch_size * extra_num_user_objects, lookup_count);
  const std::vector<uint32_t> lookups(
      minibatch_size * 10 * extra_num_user_objects, 42);
  std::vector<uint8_t> psum(minibatch_size * extra_num_user_objects *
                            sparse_feature_size * sizeof(uint32_t));

  op.set_run_params(minibatch_size, pnm::make_view(lookups_size),
                    pnm::make_view(lookups), pnm::make_view(psum));

  std::unique_ptr<pnm::sls::ReductionOperation> internal;

  internal = std::make_unique<pnm::sls::SLSOperationRanked>(
      op, *context->device()->as<pnm::sls::device::BaseDevice>());

  for (auto pack : pnm::sls::device::RankMemory::pack_identifiers()) {
    fmt::print("Execution counts for pack({:b}): {}\n", pack,
               internal->get_num_exec_iterations(pack));
    const uint32_t num_execs = internal->get_num_exec_iterations(pack);

    for (uint32_t i = 0; i < num_execs; ++i) {
      auto rank = 0U;
      while (!((pack >> rank) & 0x1) && rank < pnm::device::topo().NumOfRanks) {
        rank++;
      }

      auto trace = internal->generate_instructions(pack, rank, i);
      if (trace.size() > 10) {
        // Crop the trace size to 10 elements, just to see that it's not zero
        trace.resize(10);
      }
      fmt::print("Trace: {}\n", fmt::join(trace, " "));
    }
  }
}
