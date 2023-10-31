#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/indices_generator/factory.h"
#include "tools/datagen/sls/tables_generator/factory.h"

#include "pnmlib/sls/embedding_tables.h"
#include "pnmlib/sls/operation.h"
#include "pnmlib/sls/sls.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/runner.h"

#include "pnmlib/common/views.h"

#include <fmt/core.h>

#include <linux/sls_resources.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

int main() {
  const size_t num_tables = 1;
  const size_t sparse_feature_size = 16;
  const int idx_values = 32;
  const int table_len = 2000000;

  const std::vector<std::pair<int /*mini_batch_size*/, int /*num_lookups*/>>
      test_batch_sizes_num_lookups = {
          {16, 10},   {16, 50},   {16, 100},  {16, 500},  {16, 1000},
          {16, 2000}, {32, 10},   {32, 50},   {32, 100},  {32, 500},
          {32, 1000}, {64, 10},   {64, 50},   {64, 100},  {64, 500},
          {128, 10},  {128, 50},  {128, 100}, {256, 10},  {256, 50},
          {256, 100}, {512, 10},  {512, 50},  {640, 10},  {768, 10},
          {896, 10},  {1024, 10}, {1152, 10}, {1280, 10}, {1408, 10},
          {1536, 10}, {1664, 10}, {1792, 10}, {1920, 10}, {2048, 10}};

  const int knum_requests = 1000;

  auto table_generator =
      tools::gen::sls::TablesGeneratorFactory::default_factory().create(
          "float_tapp", std::to_string(idx_values) + " " +
                            std::to_string(sparse_feature_size));

  std::vector<uint32_t> rows(num_tables, table_len);
  const tools::gen::sls::TablesInfo tinfo(num_tables, sparse_feature_size,
                                          rows);
  auto tables = table_generator->create(tinfo);

  // Initialize AXDIMM device
  auto context = pnm::make_context(pnm::Device::Type::SLS);
  const pnm::Runner runner(context);

  const auto sparse_feature_size_bytes = sparse_feature_size * sizeof(float);

  auto indices_generator =
      tools::gen::sls::IndicesGeneratorFactory::default_factory().create(
          "random");

  auto tables_layout = pnm::memory::EmbeddingTables::create(
      pnm::views::make_view(std::cref(tables).get()), rows,
      sparse_feature_size_bytes, context, SLS_ALLOC_AUTO);

  std::vector<uint32_t> lookups_per_table;

  std::vector<std::vector<uint32_t>> indices;

  for (auto [mini_batch_size, num_lookup] : test_batch_sizes_num_lookups) {
    const auto psum_num = num_tables * mini_batch_size;
    if (psum_num > 4096) {
      fmt::print(
          stderr,
          "HW restriction: num_tables * mini_batch_size mast be lower or "
          "equal 4096, but it is equal to {}",
          psum_num);
      continue;
    }
    fmt::print("Current execution parameters:\n");
    fmt::print("\tNum lookup: {}\n", num_lookup);
    fmt::print("\tMini batch size : {}\n", mini_batch_size);

    lookups_per_table = std::vector<uint32_t>(num_tables, num_lookup);

    const tools::gen::sls::IndicesInfo info(
        std::vector<size_t>(lookups_per_table.begin(), lookups_per_table.end()),
        mini_batch_size);

    indices.clear();

    std::generate_n(std::back_inserter(indices), knum_requests,
                    [&indices_generator, &info, &tinfo]() {
                      return indices_generator->create(info, tinfo);
                    });

    pnm::profile::sls_start_profiling();

    pnm::operations::SlsOperation sls_op{
        sparse_feature_size, pnm::views::make_const_view(rows),
        tables_layout.get(), pnm::operations::SlsOperation::Type::Float};

    std::vector<float> psum(mini_batch_size * num_tables * sparse_feature_size);
    // 3. Run SLS on SLS device/simulator.

    for (int i = 0; i < knum_requests; ++i) {
      sls_op.set_run_params(
          mini_batch_size, pnm::views::make_view(info.lengths()),
          pnm::views::make_const_view(indices[i]),
          pnm::views::view_cast<uint8_t>(pnm::views::make_view(psum)));
      runner.run(sls_op);
    }

    pnm::profile::sls_report_profile();
  }

  return 0;
}
