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

#ifndef SLS_TEST_FIXTURE_H
#define SLS_TEST_FIXTURE_H

#include "secure/encryption/sls_postrocessor.h"

#include "common/compiler_internal.h"
#include "common/mapped_file.h"

#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/general/test_tables_mmap.h"
#include "tools/datagen/sls/tables_generator/base.h"
#include "tools/datagen/sls/tables_generator/factory.h"
#include "tools/datagen/sls/utils.h"

#include "pnmlib/common/128bit_math.h"
#include "pnmlib/common/rowwise_view.h"
#include "pnmlib/common/variable_row_view.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <stdlib.h> // NOLINT(modernize-deprecated-headers)

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iterator>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

/*! \brief The info aboun structure of custom embedding tables. Use to simplify
 * debug process */
struct TestEnv {
  constexpr static std::array batches_for_table = {5U, 11U, 128U, 56U};
  constexpr static uint64_t lookup_size = 32;

  //[TODO @y-lavrinenko] Use generic API
  static auto fill_indices(const tools::gen::sls::TablesInfo &emb_info) {
    std::vector<uint32_t> indices;
    auto lookups_per_table = std::accumulate(batches_for_table.begin(),
                                             batches_for_table.end(), 0UL) *
                             lookup_size;
    indices.resize(emb_info.rows().size() * lookups_per_table);
    auto i = 0;

    std::mt19937_64 reng{std::random_device{}()};

    auto indices_it = indices.begin();
    for (const auto &bcount : batches_for_table) {
      std::uniform_int_distribution index_gen{0U, emb_info.rows()[i] - 1};

      auto indices_count = bcount * lookup_size;
      std::generate(indices_it, indices_it + indices_count,
                    [&reng, &index_gen]() { return index_gen(reng); });
      std::advance(indices_it, indices_count);

      ++i;
    }
    return indices;
  }

  static auto table_sparse_sls(
      pnm::views::rowwise<const uint32_t> table,
      pnm::views::variable_row<const uint32_t, const uint32_t> indices,
      size_t cols) {
    auto plus = [](const auto &l, const auto &r)
                    NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW { return l + r; };
    std::vector<uint32_t> psums;
    psums.reserve(indices.size() * cols);

    for (auto batch : indices) {
      std::vector<uint32_t> psum(cols, 0);
      for (auto index : batch) {
        auto row = *(table.begin() + index);
        std::transform(psum.begin(), psum.end(), row.begin(), psum.begin(),
                       plus);
      }
      psums.insert(psums.end(), psum.begin(), psum.end());
    }

    return psums;
  }

  static auto table_tag_sls(
      pnm::views::rowwise<const uint32_t> tags,
      pnm::views::variable_row<const uint32_t, const uint32_t> indices)
      NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW {
    std::vector<pnm::types::uint128_t> sls_tag;
    sls_tag.reserve(indices.size());

    for (auto batch : indices) {
      pnm::types::uint128_t tag_sum = 0;
      for (auto index : batch) {
        const auto tag = *reinterpret_cast<const pnm::types::uint128_t *>(
            (tags.begin() + index)->begin());
        tag_sum += tag;
      }
      sls_tag.emplace_back(tag_sum);
    }

    return std::vector<uint32_t>(
        reinterpret_cast<uint32_t *>(sls_tag.data()),
        reinterpret_cast<uint32_t *>(sls_tag.data() + sls_tag.size()));
  }
};

using namespace pnm::sls::secure;
template <typename T, typename C> inline T &as(C *addr) {
  return *reinterpret_cast<T *>(addr);
}

/*! \brief Fixture for full process tests  */
struct SecurePostprocessorArtificial : public ::testing::Test {
  std::string root;
  std::vector<uint32_t> data;
  std::vector<uint32_t> golden;
  tools::gen::sls::TablesMmap emb_table;

  pnm::sls::secure::indices_t indices_views;
  std::vector<uint32_t> indices;
  std::vector<uint32_t> lengths;
  void SetUp() override {
    constexpr static std::array rows = {50U, 100U, 129U, 1929U};
    constexpr static size_t sparse_feature_size = 4;

    auto info = tools::gen::sls::TablesInfo{
        rows.size(), sparse_feature_size, {rows.begin(), rows.end()}};

    root = (std::filesystem::temp_directory_path() / "embXXXXXX").string();
    ASSERT_TRUE(mkdtemp(root.data()));
    std::unique_ptr<tools::gen::sls::ITablesGenerator> gen;
    ASSERT_NO_THROW(
        gen = tools::gen::sls::TablesGeneratorFactory::default_factory().create(
            "position"));
    ASSERT_NO_THROW(gen->create_and_store(root, info));

    emb_table = tools::gen::sls::get_test_tables_mmap(root);
    auto emp_table_view = emb_table.mapped_file.get_view<uint32_t>();
    data = std::vector<uint32_t>(emp_table_view.begin(), emp_table_view.end());

    indices = TestEnv::fill_indices(emb_table.info);

    auto num_requests = std::accumulate(TestEnv::batches_for_table.begin(),
                                        TestEnv::batches_for_table.end(), 0ULL);
    lengths.resize(num_requests, TestEnv::lookup_size);

    const auto *data_it = data.data();
    const auto *index_it = indices.data();
    const auto *lengths_it = lengths.data();

    for (auto tid = 0UL; tid < emb_table.info.rows().size(); ++tid) {
      auto table_view = pnm::views::make_rowwise_view(
          data_it, data_it + emb_table.info.rows()[tid] * emb_table.info.cols(),
          emb_table.info.cols());
      auto indices_view = pnm::views::make_variable_row_view(
          index_it,
          index_it + TestEnv::batches_for_table[tid] * TestEnv::lookup_size,
          pnm::views::make_view(lengths_it,
                                lengths_it + TestEnv::batches_for_table[tid]));
      indices_views.emplace_back(tid, indices_view);

      std::advance(data_it, emb_table.info.rows()[tid] * emb_table.info.cols());
      std::advance(index_it,
                   TestEnv::batches_for_table[tid] * TestEnv::lookup_size);
      auto v = TestEnv::table_sparse_sls(table_view, indices_view,
                                         emb_table.info.cols());
      golden.insert(golden.end(), v.begin(), v.end());
    }
  }

  void TearDown() override { std::filesystem::remove_all(root); }
};

struct TablePreprocessing : public ::testing::Test {
  std::string root;
  tools::gen::sls::TablesInfo info;

  size_t rows() const {
    return std::accumulate(info.rows().begin(), info.rows().end(), 0ULL);
  }

  size_t data_count() const { return info.cols() * rows(); }

  // Memory size to emulate rank memory layout. It's two time greater than
  // required for data.
  constexpr static size_t RANKS = 2;
  size_t rank_bytes() const { return data_count() * RANKS * sizeof(uint32_t); }

  size_t rank_bytes_with_tag() const {
    return (info.cols() * sizeof(uint32_t) + sizeof(__int128)) * rows() * RANKS;
  }

  void SetUp() override {
    const std::vector<uint32_t> rows_count{15U, 20U, 50U, 25U, 50U, 1024};
    constexpr static size_t sparse_feature_size = 16;
    info = tools::gen::sls::TablesInfo{rows_count.size(), sparse_feature_size,
                                       rows_count};

    root = (std::filesystem::temp_directory_path() / "embXXXXXX").string();
    ASSERT_TRUE(mkdtemp(root.data()));
    std::unique_ptr<tools::gen::sls::ITablesGenerator> gen;
    ASSERT_NO_THROW(
        gen = tools::gen::sls::TablesGeneratorFactory::default_factory().create(
            "position"));
    ASSERT_NO_THROW(gen->create_and_store(root, info));
  }

  void TearDown() override { std::filesystem::remove_all(root); }
};

#endif // SLS_TEST_FIXTURE_H
