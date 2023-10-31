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

#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/golden_vec_generator/factory.h"
#include "tools/datagen/sls/indices_generator/factory.h"
#include "tools/datagen/sls/tables_generator/factory.h"
#include "tools/datagen/sls/utils.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <stdlib.h> // NOLINT(modernize-deprecated-headers, misc-include-cleaner)

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

using namespace tools::gen::sls;

struct SlsFixture : public ::testing::Test {
protected:
  std::string root;
  void SetUp() override {
    root = (std::filesystem::temp_directory_path() / "embXXXXXX").string();
    ASSERT_TRUE(mkdtemp(root.data()));

    fmt::print("The temporary embedding tables will be saved at {}\n", root);
  }
  void TearDown() override { std::filesystem::remove_all(root); }

  void sls_test_value(uint32_t fill_value) {
    const TablesInfo tinfo(5, 4, {30, 40, 20, 100, 14});

    auto tgen = TablesGeneratorFactory::default_factory().create(
        "fixed", std::to_string(fill_value));
    tgen->create_and_store(root, tinfo);

    const IndicesInfo iinfo({5, 6, 7, 8, 9}, 5);
    auto igen = IndicesGeneratorFactory::default_factory().create("sequential");
    ASSERT_NO_THROW(igen->create_and_store(root, "seq_ok", iinfo, tinfo));

    auto indices = get_test_indices(root, "seq_ok");
    auto tables = get_test_tables_mmap(root);

    auto generator =
        GoldenVecGeneratorFactory::default_factory().create("uint32_t");
    generator->compute_and_store_golden_sls(root, "seq_ok", tables, indices);

    auto golden_ss = get_golden_vector(root, "seq_ok");
    auto psum_entries_count = 5 * 5;

    std::vector<uint32_t> value(4);
    auto golden_it = iinfo.lengths().begin();
    while (golden_ss.read(reinterpret_cast<char *>(value.data()),
                          sizeof(uint32_t) * value.size())) {
      ASSERT_EQ(std::count(value.begin(), value.end(), *golden_it * fill_value),
                4);
      --psum_entries_count;
      if (psum_entries_count % 5 == 0) {
        golden_it += iinfo.minibatch_size();
      }
    }
    ASSERT_EQ(psum_entries_count, 0);
  }
};

TEST_F(SlsFixture, SequentialOk) { sls_test_value(1); }

TEST_F(SlsFixture, SequentialOk100) { sls_test_value(10); }

TEST_F(SlsFixture, RandomOk2) {
  const TablesInfo tinfo(5, 4, {30, 40, 20, 100, 14});

  auto tgen = TablesGeneratorFactory::default_factory().create("fixed", "2");
  tgen->create_and_store(root, tinfo);

  const IndicesInfo iinfo({5, 6, 7, 8, 9}, 5);
  auto igen = IndicesGeneratorFactory::default_factory().create("random");
  ASSERT_NO_THROW(igen->create_and_store(root, "random", iinfo, tinfo));

  auto indices = get_test_indices(root, "random");
  auto tables = get_test_tables_mmap(root);

  auto generator =
      GoldenVecGeneratorFactory::default_factory().create("uint32_t");
  generator->compute_and_store_golden_sls(root, "random", tables, indices);

  auto golden_ss = get_golden_vector(root, "random");
  auto psum_entries_count = 5 * 5;

  std::vector<uint32_t> value(4);
  auto golden_it = iinfo.lengths().begin();
  while (golden_ss.read(reinterpret_cast<char *>(value.data()),
                        sizeof(uint32_t) * value.size())) {
    ASSERT_EQ(std::count(value.begin(), value.end(), *golden_it * 2), 4);
    --psum_entries_count;
    if (psum_entries_count % 5 == 0) {
      golden_it += iinfo.minibatch_size();
    }
  }
  ASSERT_EQ(psum_entries_count, 0);
}

TEST(SLS, Position) {
  const TablesInfo tinfo(3, 4, {10, 20, 5});

  // Generate 'position' embedding tables to evaluate psum by hand
  auto tgen = TablesGeneratorFactory::default_factory().create("position");
  auto emb_tables = tgen->create(tinfo);

  const IndicesInfo iinfo({4, 3, 4}, 2);
  auto igen = IndicesGeneratorFactory::default_factory().create("sequential");
  auto indices = igen->create(iinfo, tinfo);

  auto generator =
      GoldenVecGeneratorFactory::default_factory().create("uint32_t");
  auto golden = generator->compute_golden_sls(emb_tables.data(), tinfo,
                                              indices.data(), iinfo);
  const std::vector<uint32_t> very_golden{
      0x0600,   0x0604,   0x0608,   0x060C,   0x0600,   0x0604,
      0x0608,   0x060C,   0x030300, 0x030303, 0x030306, 0x030309,
      0x030300, 0x030303, 0x030306, 0x030309, 0x080600, 0x080604,
      0x080608, 0x08060C, 0x080600, 0x080604, 0x080608, 0x08060C};

  ASSERT_EQ(std::vector<uint32_t>(
                reinterpret_cast<uint32_t *>(golden.data()),
                reinterpret_cast<uint32_t *>(golden.data() + golden.size())),
            very_golden);
}
