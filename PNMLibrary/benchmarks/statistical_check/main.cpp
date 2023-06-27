/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

// NOLINTBEGIN(misc-use-anonymous-namespace)

#include "tools/datagen/sls/utils.h"

#include "pnmlib/sls/embedded_tables.h"
#include "pnmlib/sls/operation.h"
#include "pnmlib/sls/type.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/runner.h"

#include "pnmlib/common/views.h"

#include <benchmark/benchmark.h>

#include <fmt/core.h>

#include <linux/sls_resources.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

void benchmark_sls(benchmark::State &state) {
  const size_t num_tables = state.range(0);
  const uint32_t sparse_feature_size = state.range(1);
  static constexpr int idx_values = 32;
  static constexpr int table_len = 2000000;

  const int mini_batch_size = state.range(2);
  const int num_lookups = state.range(3);

  // Put this on tmpfs, which is stored in ram. This helps to get rid of
  // variations caused by disk speed
  static constexpr const char *root = "/tmp/statistical_benchmark_data";

  const std::string generator_args =
      fmt::format("{} {}", idx_values, sparse_feature_size);
  std::vector<uint8_t> tables;

  sls::tests::get_or_create_test_tables(root, "float_tapp", generator_args,
                                        sparse_feature_size, num_tables,
                                        table_len, tables);

  auto context = pnm::make_context(pnm::Device::Type::SLS_AXDIMM);

  const auto sparse_feature_size_bytes = sparse_feature_size * sizeof(float);
  std::vector<uint32_t> rows(num_tables, table_len);

  std::vector<uint32_t> lengths;
  std::vector<uint32_t> indices;

  const sls::tests::GeneratorWithOverflowParams params{
      .max_num_lookup = static_cast<size_t>(num_lookups),
      .min_num_lookup = static_cast<size_t>(num_lookups),
      .mini_batch_size = mini_batch_size,
      .root = root,
      .prefix = fmt::format("my_indices_bs_{}", mini_batch_size),
      .generator_lengths_name = "random",
      .overflow_type = "0",
      .generator_indices_name = "random",
      .entry_type = "float"};

  sls::tests::get_or_create_test_indices(params, lengths, indices);

  auto tables_layout = pnm::memory::EmbeddedTables::create(
      pnm::make_view(std::cref(tables).get()), rows, sparse_feature_size_bytes,
      context, SLS_ALLOC_AUTO);

  PNMSLSOperation sls_op{sparse_feature_size, pnm::make_const_view(rows),
                         tables_layout.get(), SLSType::Float};

  std::vector<float> psum(mini_batch_size * num_tables * sparse_feature_size);

  sls_op.set_run_params(mini_batch_size, pnm::make_const_view(lengths),
                        pnm::make_const_view(indices),
                        pnm::view_cast<uint8_t>(pnm::make_view(psum)));

  const pnm::Runner runner(context);
  for ([[maybe_unused]] auto _ : state) {
    runner.run(sls_op);
  }
}

BENCHMARK(benchmark_sls)
    ->ArgsProduct({
        // num_tables
        {1},
        // sparse_feature_size
        {16},
        // minibatch_size
        {1, 2, 8, 32, 128},
        // num_lookups
        {50},
    });

BENCHMARK_MAIN();

// NOLINTEND(misc-use-anonymous-namespace)
