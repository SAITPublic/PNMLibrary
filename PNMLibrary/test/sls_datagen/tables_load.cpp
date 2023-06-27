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

#include "common/mapped_file.h"
#include "common/temporary_filesystem_object.h"

#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/general/test_tables_mmap.h"
#include "tools/datagen/sls/general/traits.h"
#include "tools/datagen/sls/tables_generator/base.h"
#include "tools/datagen/sls/tables_generator/factory.h"
#include "tools/datagen/sls/utils.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>

TEST(TablesMmapLoad, LoadViaUtils_position) {
  const pnm::utils::TemporaryDirectory root{};
  fmt::print("The temporary embedded tables will be saved at {}\n",
             root.name());

  const TablesInfo tinfo{4, 16, {5, 20, 56, 42}}; // Some random tables
  {                                               // Prepare input data
    std::unique_ptr<ITablesGenerator> gen;
    gen = TablesGeneratorFactory::default_factory().create("position");
    gen->create_and_store(root.name(), tinfo);
  }

  ASSERT_THROW(sls::tests::get_test_tables_mmap("/"), pnm::error::IO);
  auto tables = sls::tests::get_test_tables_mmap(root.name());

  ASSERT_EQ(tables.info.rows(), tinfo.rows());
  ASSERT_EQ(tables.info.cols(), tinfo.cols());
  ASSERT_EQ(tables.info.num_tables(), tinfo.num_tables());

  auto tables_view = tables.mapped_file.get_view<uint32_t>();
  size_t ind = 0;
  for (size_t tid = 0; tid < tables.info.num_tables(); ++tid) {
    auto max_rid = tables.info.rows()[tid];
    for (size_t rid = 0; rid < max_rid; ++rid) {
      for (size_t cid = 0; cid < tables.info.cols(); ++cid, ++ind) {
        uint32_t probe;
        const uint32_t golden = (tid << 16) + (rid << 8) + cid;
        probe = tables_view.begin()[ind];
        ASSERT_EQ(probe, golden);
      }
    }
  }
}

TEST(TablesMmapLoad, LoadViaUtils_float_tapp) {
  const pnm::utils::TemporaryDirectory root{};
  fmt::print("The temporary embedded tables will be saved at {}\n",
             root.name());

  constexpr uint32_t sparse_feature_size = 16;
  constexpr uint32_t knum_idx_values = 32;
  constexpr auto kseed_sparse_feature_val =
      DataTypeTraits<float>::seed_sparse_feature_val;

  const TablesInfo tinfo{4, 16, {5, 20, 56, 42}}; // Some random tables
  {                                               // Prepare input data
    std::unique_ptr<ITablesGenerator> gen;
    auto args = fmt::format("{} {}", knum_idx_values, sparse_feature_size);
    gen = TablesGeneratorFactory::default_factory().create("float_tapp", args);
    gen->create_and_store(root.name(), tinfo);
  }

  ASSERT_THROW(sls::tests::get_test_tables_mmap("/"), pnm::error::IO);
  auto tables = sls::tests::get_test_tables_mmap(root.name());

  ASSERT_EQ(tables.info.rows(), tinfo.rows());
  ASSERT_EQ(tables.info.cols(), tinfo.cols());
  ASSERT_EQ(tables.info.num_tables(), tinfo.num_tables());

  auto tables_view = tables.mapped_file.get_view<float>();
  size_t ind = 0;
  size_t feature_id_ = 0;
  float golden;
  float probe;
  for (size_t tid = 0; tid < tables.info.num_tables(); ++tid) {
    auto max_rid = tables.info.rows()[tid];
    for (size_t rid = 0; rid < max_rid; ++rid) {
      for (size_t cid = 0; cid < tables.info.cols(); ++cid, ++ind) {
        if (ind % sparse_feature_size == 0) {
          golden =
              (feature_id_ % knum_idx_values + 1) * kseed_sparse_feature_val;
          ++feature_id_;
        }
        probe = tables_view.begin()[ind];
        ASSERT_EQ(probe, golden);
      }
    }
  }
}

TEST(TablesMmapLoad, LoadViaUtils_uint32_t_tapp) {
  const pnm::utils::TemporaryDirectory root{};
  fmt::print("The temporary embedded tables will be saved at {}\n",
             root.name());

  constexpr uint32_t sparse_feature_size = 16;
  constexpr uint32_t knum_idx_values = 32;
  constexpr auto kseed_sparse_feature_val =
      DataTypeTraits<uint32_t>::seed_sparse_feature_val;

  const TablesInfo tinfo{4, 16, {5, 20, 56, 42}}; // Some random tables
  {                                               // Prepare input data
    std::unique_ptr<ITablesGenerator> gen;
    auto args = fmt::format("{} {}", knum_idx_values, sparse_feature_size);
    gen =
        TablesGeneratorFactory::default_factory().create("uint32_t_tapp", args);
    gen->create_and_store(root.name(), tinfo);
  }

  ASSERT_THROW(sls::tests::get_test_tables_mmap("/"), pnm::error::IO);
  auto tables = sls::tests::get_test_tables_mmap(root.name());

  ASSERT_EQ(tables.info.rows(), tinfo.rows());
  ASSERT_EQ(tables.info.cols(), tinfo.cols());
  ASSERT_EQ(tables.info.num_tables(), tinfo.num_tables());

  size_t ind = 0;
  size_t feature_id_ = 0;
  uint32_t golden;
  uint32_t probe;

  auto tables_view = tables.mapped_file.get_view<uint32_t>();

  for (size_t tid = 0; tid < tables.info.num_tables(); ++tid) {
    auto max_rid = tables.info.rows()[tid];
    for (size_t rid = 0; rid < max_rid; ++rid) {
      for (size_t cid = 0; cid < tables.info.cols(); ++cid, ++ind) {
        if (ind % sparse_feature_size == 0) {
          golden =
              (feature_id_ % knum_idx_values + 1) * kseed_sparse_feature_val;
          ++feature_id_;
        }
        probe = tables_view.begin()[ind];
        ASSERT_EQ(probe, golden);
      }
    }
  }
}
