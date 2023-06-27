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

#include "secure/common/sls_io.h"
#include "secure/plain/sls.h"

#include "common/compiler_internal.h"
#include "common/topology_constants.h"

#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/indices_generator/factory.h"
#include "tools/datagen/sls/tables_generator/factory.h"

#include "pnmlib/secure/base_device.h"
#include "pnmlib/secure/base_runner.h"
#include "pnmlib/secure/untrusted_sls_params.h"

#include "pnmlib/core/device.h"
#include "pnmlib/core/sls_device.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <linux/sls_resources.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

inline constexpr size_t workers_count = 8;

using sls_element_type = uint32_t;
using SlsRunner = sls::secure::ProdConsSLSRunner<sls_element_type>;
using MemoryWriter = SlsRunner::ExecutorType::sls_device_type::MemoryWriter;

struct TableParam {
  size_t row_size;
  static constexpr size_t count = 4;
  static constexpr size_t column_count = 64;

  static TableParam calculate() {
    const auto device = SlsDevice::make(pnm::Device::Type::SLS_CXL);

    const auto size_per_worker = device.base_memory_size() /
                                 pnm::device::topo().NumOfRanks / workers_count;

    const uint32_t sparse_ft_full_size =
        sls::secure::io_traits<MemoryWriter>::memory_size<sls_element_type>(
            column_count, true);

    const TableParam table_param{
        .row_size = size_per_worker / sparse_ft_full_size / count,
    };

    return table_param;
  }
};

struct RunnerContext {
  std::unique_ptr<sls::secure::IRunner> runner;
  std::vector<sls_element_type> psum;
  std::vector<uint8_t> row_checks;
};

TEST(SecNdp, Stress) {
  if constexpr (TSAN) {
    GTEST_SKIP() << "This tests allocate all SLS memory and TSan increase "
                    "memory consumption explicitly to x8 times"
                 << '\n';
  }

  const auto table_param = TableParam::calculate();

  const std::vector<uint32_t> table_row_sizes(TableParam::count,
                                              table_param.row_size);

  const TablesInfo tinfo(table_row_sizes.size(), TableParam::column_count,
                         table_row_sizes);
  auto tgen = TablesGeneratorFactory::default_factory().create("position");
  auto emb_tables = tgen->create(tinfo);

  const std::vector<size_t> num_of_indices_per_table(TableParam::count, 16);
  static constexpr size_t minibatch_size = 2;

  IndicesInfo iinfo(num_of_indices_per_table, minibatch_size);
  auto igen = IndicesGeneratorFactory::default_factory().create("random");
  const auto indices = igen->create(iinfo, tinfo);

  const sls::secure::UntrustedDeviceParams sls_device_params{
      .rows = pnm::make_view(tinfo.rows()),
      .sparse_feature_size = tinfo.cols(),
      .with_tag = true,
      .preference = SLS_ALLOC_REPLICATE_ALL,
  };

  std::vector<RunnerContext> contexts;

  auto device_arguments = sls::secure::DeviceArguments(sls_device_params);
  auto requests_count = tinfo.num_tables() * iinfo.minibatch_size();
  for (;;) {
    try {
      auto &ctx = contexts.emplace_back();

      ctx.runner = std::make_unique<SlsRunner>();
      ctx.psum = std::vector<sls_element_type>(tinfo.cols() * requests_count);
      ctx.row_checks = std::vector<uint8_t>(requests_count, false);

      ctx.runner->init(&device_arguments);

      ctx.runner->load_tables(emb_tables.data(), pnm::make_view(tinfo.rows()),
                              tinfo.cols(), true);

    } catch (pnm::error::OutOfMemory &e) {
      contexts.pop_back();
      break;
    } catch (...) {
      throw;
    }
  }

  // because GenAlloc allocate more space that requested,
  // the runners count may be equal or less that 'workers_count'
  ASSERT_LE(workers_count, contexts.size() + 1);

  std::vector<std::thread> workers(contexts.size());
  std::atomic<size_t> barrier_cnt = workers.size();

  size_t idx = 0;
  for (auto &worker : workers) {
    worker =
        std::thread([&barrier_cnt, &iinfo, &indices, &ctx = contexts[idx++]]() {
          auto &[runner, psum, row_checks] = ctx;

          /* wait until all thread initialized*/
          for (--barrier_cnt; barrier_cnt != 0;) {
          }

          const bool verification_result = runner->run(
              iinfo.minibatch_size(), pnm::make_view(iinfo.lengths()),
              pnm::make_view(indices),
              pnm::view_cast<uint8_t>(pnm::make_view(psum)),
              pnm::make_view(row_checks));

          EXPECT_TRUE(verification_result);
        });
  }

  for (auto &worker : workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}
