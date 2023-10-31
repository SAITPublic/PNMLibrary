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

#include <pnmlib/sls/embedding_tables.h>
#include <pnmlib/sls/operation.h>

#include <pnmlib/core/context.h>
#include <pnmlib/core/device.h>
#include <pnmlib/core/runner.h>

#include <pnmlib/common/views.h>

#include <fmt/core.h>
#include <fmt/format.h>

#include <linux/sls_resources.h>

#include <cstdint>
#include <numeric>
#include <vector>

/** @brief Make embedding tables with predefined values
 *
 * Make `tables_count` embedding tables with `rows_number` rows and
 * `sparse_feature_size` cols. Each rows contains line in next pattern
 * 000...0000 TABLE_ID+1 ROW_ID+1  COL_ID+1
 * |<-8bit->||<--8bit-->|<-8bit->|<-8bit->|
 *
 * This layout allows to catch the error in SLS instruction generation, because
 * we can identify origin tables and rows in SLS-result.
 *
 * We don't use data generation here because we need clear input data.
 */
std::vector<float> make_embedding_tables(uint32_t tables_count,
                                         uint32_t rows_number,
                                         uint32_t sparse_feature_size) {
  std::vector<float> etable(tables_count * rows_number * sparse_feature_size);

  auto it = etable.begin();
  for (auto i = 0UL; i < tables_count; ++i) {
    for (auto j = 0UL; j < rows_number; ++j) {
      std::iota(it, it + sparse_feature_size, ((i + 1) << 16) + (j << 8) + 1);
      it += sparse_feature_size;
    }
  }

  return etable;
}

/** @brief Is used to cast float range to uint32_t and print by 8 bit values */
template <typename T> void print_range_hexed(T begin, T end) {
  while (begin != end) {
    const auto value = static_cast<uint32_t>(*begin);
    const auto cols_value = value & 0xFF;
    const auto rows_value = (value >> 8) & 0xFF;
    const auto table_value = (value >> 16) & 0xFF;
    fmt::print("{}'{}'{} ", table_value, rows_value, cols_value);
    ++begin;
  }
  fmt::print("\n");
}

int main() {
  static constexpr auto tables_count = 3;
  static constexpr auto rows_number = 10;
  static constexpr auto sparse_feature_size = 16;
  const std::vector<uint32_t> TSizes(tables_count, rows_number);

  auto etable =
      make_embedding_tables(tables_count, rows_number, sparse_feature_size);

  //*********************PRINT EMBEDDED TABLES*********************************
  {
    auto it = etable.begin();
    fmt::print("Embedding tables:\n");
    for (auto i = 0UL; i < tables_count; ++i) {
      for (auto j = 0UL; j < rows_number; ++j) {
        print_range_hexed(it, it + sparse_feature_size);
        it += sparse_feature_size;
      }
      fmt::print("\n");
    }
  }
  //***************************************************************************

  auto context = pnm::make_context(pnm::Device::Type::SLS);

  // DISTRIBUTE_ALL policy to utilize all available CUNITs
  auto embedding_tables = pnm::memory::EmbeddingTables::create(
      pnm::views::make_const_view(etable), TSizes,
      sparse_feature_size * sizeof(float), context, SLS_ALLOC_DISTRIBUTE_ALL);

  //***********************PRINT BUFFER LAYOUT*********************************
  for (const auto &buffer : embedding_tables->buffers()) {
    auto addr = std::get<pnm::memory::RankedRegion>(buffer.device_region());
    fmt::print("Alloc buffer on: Channel, Address\n");
    for (auto &r : addr.regions) {
      if (r.location.has_value()) {
        fmt::print("\t\t{}, {}\n", *r.location, r.start);
      }
    }
  }
  //***************************************************************************

  static constexpr auto NBatches = 2;
  static constexpr auto NLookup = 4;
  std::vector<uint32_t> lengths(tables_count * NBatches, NLookup);
  std::vector<uint32_t> indices(tables_count * NBatches * NLookup, 0);

  auto index_iter = 0;
  // Select indices from 0 with step 3. Indices are generated for ALL tables
  for (auto &index : indices) {
    index = index_iter;
    index_iter = (index_iter + 3) % rows_number;
  }

  //***********************PRINT REQUESTS INFORMATION**************************
  fmt::print("Requests:\n");
  fmt::print("Lengths: {}\n", fmt::join(lengths, " "));
  fmt::print("Indices:\n");
  auto indices_it = indices.begin();
  for (auto i = 0UL; i < tables_count; ++i) {
    fmt::print("\tTable {}: ", i);
    for (auto batch_id = 0; batch_id < NBatches; ++batch_id) {
      fmt::print("{} | ", fmt::join(indices_it, indices_it + NLookup, " "));
      indices_it += NLookup;
    }
    fmt::print("\n");
  }
  //***************************************************************************

  pnm::operations::SlsOperation op(
      sparse_feature_size, pnm::views::make_view(TSizes),
      embedding_tables.get(), pnm::operations::SlsOperation::Type::Float);

  std::vector<float> psum(tables_count * NBatches * sparse_feature_size, 0);

  op.set_run_params(
      NBatches, pnm::views::make_const_view(lengths),
      pnm::views::make_const_view(indices),
      pnm::views::view_cast<uint8_t>(pnm::views::make_view(psum)));

  const pnm::Runner runner(context);
  runner.run(op);

  //**************************PRINT SLS RESULTS********************************
  fmt::print("Result:\n");

  auto it = psum.begin();
  for (auto i = 0; i < tables_count; ++i) {
    fmt::print("Table {}\n", i);
    for (auto j = 0; j < NBatches; ++j) {
      print_range_hexed(it, it + sparse_feature_size);
      it += sparse_feature_size;
    }
  }
  //***************************************************************************

  return 0;
}
