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

#include "common/topology_constants.h"

#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/indices_generator/factory.h"
#include "tools/datagen/sls/lengths_generator/factory.h"
#include "tools/datagen/sls/tables_generator/factory.h"
#include "tools/datagen/sls/utils.h"

#include "pnmlib/common/misc_utils.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <stdlib.h> // NOLINT(modernize-deprecated-headers)

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

struct IndicesFixture : public ::testing::Test {
protected:
  std::string root;
  void SetUp() override {
    root = (std::filesystem::temp_directory_path() / "embXXXXXX").string();
    ASSERT_TRUE(mkdtemp(root.data()));

    fmt::print("The temporary embedded tables will be saved at {}\n", root);
  }
  void TearDown() override { std::filesystem::remove_all(root); }
};

TEST_F(IndicesFixture, SequentialFail) {
  const TablesInfo tinfo(5, 16, {30, 40, 20, 100, 14});
  auto tgen = TablesGeneratorFactory::default_factory().create("random");
  tgen->create_and_store(root, tinfo);

  const IndicesInfo iinfo({15, 20, 21, 50, 9}, 10);
  auto igen = IndicesGeneratorFactory::default_factory().create("sequential");
  ASSERT_THROW(igen->create_and_store(root, "seq_fail", iinfo, tinfo),
               std::logic_error);
}

TEST_F(IndicesFixture, SequentialOk) {
  const TablesInfo tinfo(5, 16, {30, 40, 20, 100, 14});
  auto tgen = TablesGeneratorFactory::default_factory().create("random");
  tgen->create_and_store(root, tinfo);

  const IndicesInfo iinfo({15, 20, 10, 50, 9}, 10);
  auto igen = IndicesGeneratorFactory::default_factory().create("sequential");
  ASSERT_NO_THROW(igen->create_and_store(root, "seq_ok", iinfo, tinfo));

  auto indices = sls::tests::get_test_indices(root, "seq_ok");
  ASSERT_EQ(indices.info.minibatch_size(), 10);
  auto gvec = std::vector<size_t>{15, 20, 10, 50, 9};
  auto num_requests_ = gvec.size() * indices.info.minibatch_size();
  std::vector<uint32_t> lengths;
  lengths.reserve(num_requests_);

  for (const auto &lookups : gvec) {
    lengths.insert(lengths.end(), indices.info.minibatch_size(), lookups);
  }
  ASSERT_EQ(indices.info.lengths(), lengths);

  for (auto tid = 0U; tid < tinfo.num_tables(); ++tid) {
    auto bcount = indices.info.minibatch_size();
    for (auto bid = 0U; bid < bcount; ++bid) {
      std::vector<uint32_t> idx(indices.info.lengths()[tid * bcount + bid]);

      std::vector<uint32_t> golden(indices.info.lengths()[tid * bcount + bid]);
      std::iota(golden.begin(), golden.end(), 0);

      indices.in.read(reinterpret_cast<char *>(idx.data()),
                      pnm::utils::byte_size_of_container(idx));
      ASSERT_EQ(golden, idx);
    }
  }
}

TEST_F(IndicesFixture, Random) {
  const TablesInfo tinfo(5, 16, {30, 40, 20, 100, 14});
  auto tgen = TablesGeneratorFactory::default_factory().create("random");
  tgen->create_and_store(root, tinfo);

  const IndicesInfo iinfo({15, 20, 10, 50, 9}, 10);
  auto igen = IndicesGeneratorFactory::default_factory().create("random");
  ASSERT_NO_THROW(igen->create_and_store(root, "seq_ok", iinfo, tinfo));

  auto indices = sls::tests::get_test_indices(root, "seq_ok");
  ASSERT_EQ(indices.info.minibatch_size(), 10);
  auto gvec = std::vector<size_t>{15, 20, 10, 50, 9};
  auto num_requests_ = gvec.size() * indices.info.minibatch_size();
  std::vector<uint32_t> lengths;
  lengths.reserve(num_requests_);

  for (const auto &lookups : gvec) {
    lengths.insert(lengths.end(), indices.info.minibatch_size(), lookups);
  }
  ASSERT_EQ(indices.info.lengths(), lengths);

  auto rows_it = tinfo.rows().begin();
  for (auto tid = 0U; tid < tinfo.num_tables(); ++tid) {
    auto bcount = indices.info.minibatch_size();
    for (auto bid = 0U; bid < bcount; ++bid) {
      std::vector<uint32_t> idx(indices.info.lengths()[tid * bcount + bid]);
      indices.in.read(reinterpret_cast<char *>(idx.data()),
                      pnm::utils::byte_size_of_container(idx));
      ASSERT_TRUE(std::none_of(idx.begin(), idx.end(),
                               [rows_it](auto e) { return e >= *rows_it; }));
    }
    ++rows_it;
  }
}

TEST_F(IndicesFixture, Random_Lengths_0) {
  const TablesInfo tinfo(5, 16, {30, 40, 20, 100, 14});
  auto tgen = TablesGeneratorFactory::default_factory().create("random");
  tgen->create_and_store(root, tinfo);

  auto length_generator =
      LengthsGeneratorFactory::default_factory().create("random", "0");

  const size_t minibatch_size = 16;

  auto lengths =
      length_generator->create(tinfo.num_tables(), 1, 50, minibatch_size);
  const size_t num_requests = tinfo.num_tables() * minibatch_size;

  ASSERT_EQ(lengths.size(), num_requests);

  for (size_t idx = 0; idx < num_requests; ++idx) {
    ASSERT_TRUE((1 <= lengths[idx] && lengths[idx] <= 50));
  }

  const IndicesInfo info(minibatch_size, lengths);

  ASSERT_EQ(info.num_requests(), num_requests);
  ASSERT_EQ(info.minibatch_size(), minibatch_size);
}

TEST_F(IndicesFixture, Random_Lengths_3) {
  const TablesInfo tinfo(5, 16, {30, 40, 20, 100, 14});
  auto tgen = TablesGeneratorFactory::default_factory().create("random");
  tgen->create_and_store(root, tinfo);

  auto length_generator =
      LengthsGeneratorFactory::default_factory().create("random", "3");

  const size_t minibatch_size = 16;

  auto lengths =
      length_generator->create(tinfo.num_tables(), 1, 50, minibatch_size);
  const size_t num_requests = tinfo.num_tables() * minibatch_size;

  ASSERT_EQ(lengths.size(), num_requests);

  const uint32_t single_batch_lookups_min =
      (pnm::device::topo().InstBufSize / pnm::device::topo().InstructionSize /
       minibatch_size) +
      1;

  for (size_t table_idx = 0; table_idx < tinfo.num_tables(); ++table_idx) {
    for (size_t row_idx = 0; row_idx < minibatch_size; ++row_idx) {
      const size_t idx = table_idx * minibatch_size + row_idx;
      if ((table_idx + 1) % 3 == 0) {
        ASSERT_EQ(lengths[idx], single_batch_lookups_min);
      } else {
        ASSERT_TRUE((1 <= lengths[idx] && lengths[idx] <= 50));
      }
    }
  }

  const IndicesInfo info(minibatch_size, lengths);

  ASSERT_EQ(info.num_requests(), num_requests);
  ASSERT_EQ(info.minibatch_size(), minibatch_size);
}

TEST_F(IndicesFixture, Random_Lengths_minus1) {
  const TablesInfo tinfo(5, 16, {30, 40, 20, 100, 14});
  auto tgen = TablesGeneratorFactory::default_factory().create("random");
  tgen->create_and_store(root, tinfo);

  auto length_generator =
      LengthsGeneratorFactory::default_factory().create("random", "-1");

  const size_t minibatch_size = 16;

  auto lengths =
      length_generator->create(tinfo.num_tables(), 1, 50, minibatch_size);
  const size_t num_requests = tinfo.num_tables() * minibatch_size;

  ASSERT_EQ(lengths.size(), num_requests);

  const uint32_t single_overflowed_table_batch_lookups =
      (pnm::device::topo().InstBufSize / pnm::device::topo().NumOfInstBuf /
       pnm::device::topo().InstructionSize) -
      1;

  ASSERT_EQ(lengths.size(), num_requests);

  for (size_t idx = 0; idx < num_requests; ++idx) {
    ASSERT_TRUE(((1 <= lengths[idx] && lengths[idx] <= 50) ||
                 (lengths[idx] == single_overflowed_table_batch_lookups)));
  }

  const IndicesInfo info(minibatch_size, lengths);

  ASSERT_EQ(info.num_requests(), num_requests);
  ASSERT_EQ(info.minibatch_size(), minibatch_size);
}
