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

#include "secure/common/sls_io.h"
#include "secure/plain/device/trivial_cpu.h"
#include "secure/plain/device/untrusted_sls.h"
#include "secure/plain/sls.h"

#include "common/mapped_file.h"

#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/general/test_tables_mmap.h"
#include "tools/datagen/sls/golden_vec_generator/factory.h"
#include "tools/datagen/sls/indices_generator/base.h"
#include "tools/datagen/sls/indices_generator/factory.h"
#include "tools/datagen/sls/tables_generator/base.h"
#include "tools/datagen/sls/tables_generator/factory.h"
#include "tools/datagen/sls/utils.h"
#include "tools/test_utils/device_params_gen.h"

#include "pnmlib/common/128bit_math.h"
#include "pnmlib/common/misc_utils.h"
#include "pnmlib/common/rowwise_view.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest-typed-test.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-type-util.h>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <stdlib.h> // NOLINT(modernize-deprecated-headers)

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <iterator>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

using namespace sls::secure;

template <typename T> T as(void *ptr) { return static_cast<T>(ptr); }

TEST(MemIO, ReadWrite) {
  static constexpr auto size = 100;
  std::vector<uint32_t> data(size);
  std::iota(data.begin(), data.end(), 0);

  sls::secure::TrivialMemoryReader in(data.data());

  std::vector<uint32_t> out_data(size, 0);
  sls::secure::TrivialMemoryWriter out(out_data.data());

  for (auto i = 0; i < size; i += 10) {
    std::array<uint32_t, 10> slice{};
    in.read(as<char *>(slice.data()), 10 * sizeof(uint32_t));
    ASSERT_TRUE(std::equal(slice.begin(), slice.end(), data.data() + i));

    for (auto e : slice) {
      out.write(as<const char *>(&e), sizeof(e));
    }
  }

  ASSERT_EQ(data, out_data);
}

template <typename TArgs>
struct SmallTablesProcessing : public ::testing::Test {
  using runner_type = typename TArgs::first;
  using device_type = typename TArgs::second;

  std::string root;
  static constexpr auto prefix = "random_set";
  struct {
    TablesInfo tables;
    IndicesInfo indices;
  } info;

  // Prepare small embedded tables, indices and golden vectors
  void SetUp() override {
    const std::vector<uint32_t> rows_count{150U, 210U, 150U, 150U, 150U, 1024};
    constexpr static size_t sparse_feature_size = 16;
    info.tables =
        TablesInfo{rows_count.size(), sparse_feature_size, rows_count};
    info.indices = IndicesInfo{{12, 12, 12, 12, 12, 12}, 10};

    root = (std::filesystem::temp_directory_path() / "embXXXXXX").string();
    ASSERT_TRUE(mkdtemp(root.data()));

    auto tgen = TablesGeneratorFactory::default_factory().create("position");
    ASSERT_NO_THROW(tgen->create_and_store(root, info.tables));

    auto igen = IndicesGeneratorFactory::default_factory().create("random");
    ASSERT_NO_THROW(
        igen->create_and_store(root, prefix, info.indices, info.tables));

    auto generator =
        GoldenVecGeneratorFactory::default_factory().create("uint32_t");

    auto tables = sls::tests::get_test_tables_mmap(root);
    auto indices = sls::tests::get_test_indices(root, prefix);
    ASSERT_NO_THROW(
        generator->compute_and_store_golden_sls(root, prefix, tables, indices));
  }

  void TearDown() override { std::filesystem::remove_all(root); }

protected:
  static constexpr auto TAG_SIZE = sizeof(pnm::uint128_t) / sizeof(uint32_t);

  template <typename C> auto reduce(const C &c) {
    return std::accumulate(c.begin(), c.end(), 0UL);
  };

  void test_impl(bool with_tag);

  auto load_indices() {
    auto indices = sls::tests::get_test_indices(root, prefix);
    std::vector<uint32_t> indices_data(reduce(indices.info.lengths()));
    indices.in.read(reinterpret_cast<char *>(indices_data.data()),
                    static_cast<std::streamsize>(
                        pnm::utils::byte_size_of_container(indices_data)));
    return std::make_pair(std::move(indices), std::move(indices_data));
  }
};

using RunnerTypes = ::testing::Types<
    pnm::utils::TypePair<SyncCPURunner<uint32_t>, TrivialCPU<uint32_t>>,
    pnm::utils::TypePair<ProdConsCPURunner<uint32_t>, TrivialCPU<uint32_t>>,
    pnm::utils::TypePair<SyncSLSRunner<uint32_t>, UntrustedDevice<uint32_t>>,
    pnm::utils::TypePair<ProdConsSLSRunner<uint32_t>,
                         UntrustedDevice<uint32_t>>>;

TYPED_TEST_SUITE(SmallTablesProcessing, RunnerTypes);

TYPED_TEST(SmallTablesProcessing, OpExecCPULoadNoTagAsanCheck) {
  auto emb_tables = sls::tests::get_test_tables_mmap(this->root);

  typename TestFixture::runner_type runner;

  auto params = sls::tests::SimpleDeviceParamsGenerator<
      typename TestFixture::device_type>::
      generate_params(pnm::make_view(emb_tables.info.rows()),
                      emb_tables.info.cols(), false);

  runner.init(&params);
  runner.load_tables(
      reinterpret_cast<const char *>(emb_tables.mapped_file.data()),
      pnm::make_view(emb_tables.info.rows()), emb_tables.info.cols(), false);
}

TYPED_TEST(SmallTablesProcessing, OpExecCPULoadTagAsanCheck) {
  auto emb_tables = sls::tests::get_test_tables_mmap(this->root);

  typename TestFixture::runner_type runner;

  auto params = sls::tests::SimpleDeviceParamsGenerator<
      typename TestFixture::device_type>::
      generate_params(pnm::make_view(emb_tables.info.rows()),
                      emb_tables.info.cols(), true);

  runner.init(&params);
  runner.load_tables(
      reinterpret_cast<const char *>(emb_tables.mapped_file.data()),
      pnm::make_view(emb_tables.info.rows()), emb_tables.info.cols(), true);
}

template <typename TArgs>
void SmallTablesProcessing<TArgs>::test_impl(bool with_tag) {
  auto emb_tables = sls::tests::get_test_tables_mmap(root);
  auto [indices, indices_data] = load_indices();
  auto golden = sls::tests::get_golden_vector(root, prefix);

  runner_type runner;
  auto params =
      sls::tests::SimpleDeviceParamsGenerator<device_type>::generate_params(
          pnm::make_view(emb_tables.info.rows()), emb_tables.info.cols(),
          with_tag);

  runner.init(&params);
  runner.load_tables(
      reinterpret_cast<const char *>(emb_tables.mapped_file.data()),
      pnm::make_view(emb_tables.info.rows()), emb_tables.info.cols(), with_tag);

  auto sls_requests = indices.info.num_requests();
  std::vector<uint32_t> psum;
  psum.resize(sls_requests * emb_tables.info.cols(), 0);

  std::vector<uint8_t> status(sls_requests);
  auto res = runner.run(
      indices.info.minibatch_size(), pnm::make_view(indices.info.lengths()),
      pnm::make_view(std::cref(indices_data).get()),
      pnm::view_cast<uint8_t>(pnm::make_view(psum)), pnm::make_view(status));

  EXPECT_TRUE(res);

  for (auto e : psum) {
    uint32_t golden_v = 0;
    ASSERT_NO_THROW(
        golden.read(reinterpret_cast<char *>(&golden_v), sizeof(golden_v)));
    EXPECT_EQ(golden_v, e);
  }

  if (with_tag) {
    EXPECT_TRUE(
        std::all_of(status.begin(), status.end(), [](auto v) { return v; }));
  }
}

TYPED_TEST(SmallTablesProcessing, SecureApiSLSNoTag) { this->test_impl(false); }

TYPED_TEST(SmallTablesProcessing, SecureApiSLSWithTag) {
  this->test_impl(true);
}

template <typename TArgs>
struct OperationExecutorSimple : public ::testing::Test {
  using runner_type = typename TArgs::first;
  using device_type = typename TArgs::second;

  std::vector<int32_t> data;

  const std::vector<uint32_t> indices{1, 3, 4, 0, 1, 4, 0, 1, 2, 2, 3, 4, 4, 5,
                                      6, 1, 4, 9, 0, 1, 3, 1, 2, 3, 0, 1, 4};

  const std::vector<uint32_t> golden{
      56,  69,  81,  93,  160, 180, 201, 222, 173, 204, 251, 282, 63,  74,  75,
      86,  7,   11,  14,  17,  21,  32,  44,  56,  118, 130, 148, 160, 81,  91,
      91,  101, 177, 170, 184, 197, 81,  112, 153, 184, 125, 146, 167, 188, 178,
      197, 216, 235, 166, 175, 177, 190, 74,  97,  131, 162, 43,  54,  115, 126,
      137, 149, 68,  77,  216, 235, 226, 228, 251, 273, 220, 260, 310, 350, 180,
      220, 270, 310, 160, 180, 186, 215, 246, 278, 71,  93,  110, 140, 310, 320,
      410, 440, 240, 280, 230, 270, 120, 140, 220, 240, 341, 342, 233, 264, 181,
      222, 123, 144, 15,  26,  37,  49,  68,  77,  86,  95,  177, 170, 184, 197,
      81,  112, 153, 184, 125, 146, 167, 188, 180, 200, 280, 300, 221, 242, 133,
      164, 271, 302, 183, 194, 65,  86,  117, 139};

  const std::vector<uint32_t> num_entries{10UL, 15UL, 10UL};
  std::vector<uint32_t> num_indices_per_lookup{3U, 3U, 3U};
  const std::vector<uint32_t> length;
  const uint16_t sparse_feature_size = 16;
  const uint16_t tables_nb = 3;
  const uint16_t minibatch_size = 3;

  OperationExecutorSimple() : length(indices.size() / 3, 3){};

protected:
  void SetUp() override {
    data.clear();
    // Array size in the loop is 70 integer entries, we aim for 70 * 8 = 560
    // entries, to get tables_nb(10 + 15 + 10 = 35) * sparse_feature_size(16) =
    // 560, to have valid data for 35 entries (total tables rows)
    for (size_t i = 0; i < 8; ++i) {
      data.insert(
          std::begin(data),
          {
              1,  2,  3,   4,   11,  12,  13,  14,  5,  6,  7,   8,   18,  17,
              16, 15, 6,   8,   11,  13,  10,  20,  30, 40, 110, 120, 130, 140,
              50, 60, 70,  80,  170, 160, 170, 180, 60, 80, 110, 130, 10,  20,
              30, 40, 110, 120, 130, 140, 50,  60,  70, 80, 150, 160, 170, 180,
              60, 80, 110, 130, 0,   0,   0,   0,   0,  1,  0,   0,   0,   0,
          });
    }
    const std::vector<uint32_t> rows{num_entries.begin(), num_entries.end()};
  }
};
TYPED_TEST_SUITE(OperationExecutorSimple, RunnerTypes);

TYPED_TEST(OperationExecutorSimple, SLSNoTag) {
  typename TestFixture::runner_type runner;

  auto params = sls::tests::SimpleDeviceParamsGenerator<
      typename TestFixture::device_type>::
      generate_params(pnm::make_view(this->num_entries),
                      this->sparse_feature_size, false);
  runner.init(&params);
  runner.load_tables(this->data.data(), pnm::make_view(this->num_entries),
                     this->sparse_feature_size, false);

  std::vector<uint32_t> psum(
      this->sparse_feature_size * this->tables_nb * this->minibatch_size, 0);

  runner.run(this->minibatch_size, pnm::make_view(this->length),
             pnm::make_view(this->indices),
             pnm::view_cast<uint8_t>(pnm::make_view(psum)));

  ASSERT_EQ(psum, this->golden);
}

TYPED_TEST(OperationExecutorSimple, SLSWithTag) {
  typename TestFixture::runner_type runner;

  auto params = sls::tests::SimpleDeviceParamsGenerator<
      typename TestFixture::device_type>::
      generate_params(pnm::make_view(this->num_entries),
                      this->sparse_feature_size, true);
  runner.init(&params);
  runner.load_tables(this->data.data(), pnm::make_view(this->num_entries),
                     this->sparse_feature_size, true);

  std::vector<uint32_t> psum(
      this->sparse_feature_size * this->tables_nb * this->minibatch_size, 0);

  std::vector<uint8_t> checks(psum.size() / this->sparse_feature_size, false);
  auto result = runner.run(this->minibatch_size, pnm::make_view(this->length),
                           pnm::make_view(this->indices),
                           pnm::view_cast<uint8_t>(pnm::make_view(psum)),
                           pnm::make_view(checks));

  EXPECT_TRUE(result);
  EXPECT_TRUE(
      std::all_of(checks.begin(), checks.end(), [](auto v) { return v; }));
  ASSERT_EQ(psum, this->golden);
}

TYPED_TEST(OperationExecutorSimple, SLSWithTagCorrupted) {
  // With existing code we cann't corrupt data after offloading to device
  // The simplest way to corrupt data is to create dataset with overflow
  std::vector bad_data{1,   2,   3,   4,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       -1,  -2,  -3,  -4,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       5,   6,   7,   8,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       -5,  -6,  -7,  -8,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       6,   8,   11,  13,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

                       10,  20,  30,  40,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       -10, -20, -30, -40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       50,  60,  70,  80,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       -50, -60, -70, -80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       60,  80,  110, 130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       10,  20,  30,  40,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       -10, -20, -30, -40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       50,  60,  70,  80,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       -50, -60, -70, -80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       60,  80,  110, 130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   1,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   1,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       1,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       1,   1,   1,   1,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  std::vector<uint32_t> bad_golden{
      0,   0,   1,   1,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      6,   8,   11,  13,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      5,   6,   7,   8,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

      60,  80,  110, 130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      60,  80,  110, 130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      110, 140, 190, 220, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

      1,   1,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      1,   1,   1,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      1,   2,   1,   1,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  typename TestFixture::runner_type runner;

  const std::vector<uint32_t> rows{10, 15, 10};
  const std::vector<uint32_t> num_entries_wa{std::begin(rows), std::end(rows)};

  auto params = sls::tests::SimpleDeviceParamsGenerator<
      typename TestFixture::device_type>::
      generate_params(pnm::make_view(rows), this->sparse_feature_size, true);
  runner.init(&params);

  runner.load_tables(bad_data.data(), pnm::make_view(rows),
                     this->sparse_feature_size, true);

  std::vector<uint32_t> psum(
      this->minibatch_size * this->tables_nb * this->sparse_feature_size, 0);

  std::vector<uint8_t> checks(psum.size() / this->sparse_feature_size, false);
  auto result = runner.run(this->minibatch_size, pnm::make_view(this->length),
                           pnm::make_view(this->indices),
                           pnm::view_cast<uint8_t>(pnm::make_view(psum)),
                           pnm::make_view(checks));

  EXPECT_FALSE(result);

  auto golden_v = pnm::make_rowwise_view(bad_golden.data(),
                                         bad_golden.data() + bad_golden.size(),
                                         this->sparse_feature_size);
  auto psum_rv = pnm::make_rowwise_view(psum.data(), psum.data() + psum.size(),
                                        this->sparse_feature_size);

  auto it1 = golden_v.begin();
  auto it2 = psum_rv.begin();
  auto rid = 0;
  for (; it1 != golden_v.end(); ++it1, ++it2) {
    if (rid >= 6) {
      const auto dump = fmt::format("{} <==> {}", fmt::join(*it1, "\t"),
                                    fmt::join(*it2, "\t"));

      EXPECT_TRUE(std::equal(it1->begin(), it1->end(), it2->begin())) << dump;
      EXPECT_TRUE(checks[rid]);
    } else {
      EXPECT_FALSE(checks[rid]);
    }
  }
}
