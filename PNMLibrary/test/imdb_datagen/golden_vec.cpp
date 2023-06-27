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

#include "imdb/software_scan/avx2/scan.h"

#include "common/temporary_filesystem_object.h"

#include "tools/datagen/imdb/column_generator/factory.h"
#include "tools/datagen/imdb/general/bit_vectors_io.h"
#include "tools/datagen/imdb/general/columns_io.h"
#include "tools/datagen/imdb/general/index_vectors_io.h"
#include "tools/datagen/imdb/general/predicate_vecs_info.h"
#include "tools/datagen/imdb/general/predicate_vecs_io.h"
#include "tools/datagen/imdb/general/ranges_info.h"
#include "tools/datagen/imdb/general/ranges_io.h"
#include "tools/datagen/imdb/general/vector_of_vectors_info.h"
#include "tools/datagen/imdb/golden_vec_generator/gen.h"
#include "tools/datagen/imdb/predicate_vec_generator/factory.h"
#include "tools/datagen/imdb/range_generator/factory.h"
#include "tools/datagen/imdb/utils.h"

#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

using namespace tools::gen::imdb;

void assert_eq_scan_ranges(const Ranges &lhs, const Ranges &rhs) {
  ASSERT_TRUE(std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                         [](const auto &l, const auto &r) {
                           return l.start == r.start && l.end == r.end;
                         }));
}

class ImdbGoldenStoreLoad : public ::testing::Test {
protected:
  void SetUp() override {
    root_path = std::filesystem::path(root.name());
    fmt::print("The temporary embedded tables will be saved at {}\n",
               root.name());
    column_params.common_params_ = CommonGeneratorParams{
        .root_ = root_path,
        .filename_prefix_ = std::nullopt,
        .generator_name_ = literals::column_random_generator};
    column_params.entry_bit_size_ = 18;
    column_params.entry_count_ = 100'000;
    column_params.seed_ = 234;

    column = get_or_create_test_columns(column_params);
    ASSERT_EQ(column.value_bits(), column_params.entry_bit_size_);
    ASSERT_EQ(column.size(), column_params.entry_count_);

    const auto column_path = root_path / column_default_name;
    const auto column_info_path = root_path / column_info_default_name;
    const ColumnsMmap column_mmap(column_path, column_info_path);

    ASSERT_EQ(column_mmap.info.entry_bit_size(), column_params.entry_bit_size_);
    ASSERT_EQ(column_mmap.info.entry_count(), column_params.entry_count_);
    const auto loaded_column = ColumnsIO::load_from_file(column_mmap);
    ASSERT_EQ(loaded_column.size(), column.size());
    ASSERT_EQ(loaded_column.value_bits(), column.value_bits());
    for (size_t i = 0; i < column.size(); ++i) {
      ASSERT_EQ(column.get_value(i), loaded_column.get_value(i));
    }

    indexvec_path = root_path / (prefix + literals::index_vec_suffix_bin);
    indexvec_info_path = root_path / (prefix + literals::index_vec_suffix_info);
    bitvec_path = root_path / (prefix + literals::bit_vec_suffix_bin);
    bitvec_info_path = root_path / (prefix + literals::bit_vec_suffix_info);
  }

  const pnm::utils::TemporaryDirectory root{};
  std::filesystem::path root_path;
  compressed_vector column;
  GeneratorColumnParams column_params;
  std::string prefix{"prefix"};
  std::filesystem::path indexvec_path;
  std::filesystem::path indexvec_info_path;
  std::filesystem::path bitvec_path;
  std::filesystem::path bitvec_info_path;
};

TEST_F(ImdbGoldenStoreLoad, GoldenTestScanRanges) {
  // first iteration for case when generated data doesn't exist
  // second iteration tests reading already existing data
  for (int iteration = 0; iteration < 2; ++iteration) {
    GeneratorRangesParams ranges_params;
    ranges_params.common_params_ = CommonGeneratorParams{
        .root_ = root_path,
        .filename_prefix_ = prefix,
        .generator_name_ = literals::scan_ranges_generator_random};
    ranges_params.seed_ = 345;
    ranges_params.selectivities_ = std::vector<double>{0.1, 0.3, 0.5, 0.7};

    const auto ranges = get_or_create_test_scan_ranges(ranges_params);
    ASSERT_EQ(ranges.size(), ranges_params.selectivities_.size());

    const RangesMmap ranges_mmap = get_mmap_scan_ranges(root_path, prefix);
    ASSERT_EQ(ranges_mmap.info.selectivities(), ranges_params.selectivities_);

    const auto loaded_ranges = RangesIO::load_from_file(ranges_mmap);
    assert_eq_scan_ranges(loaded_ranges, ranges);

    const VectorOfVectorsMmap indexvecs_mmap(indexvec_path, indexvec_info_path);
    const VectorOfVectorsMmap bitvecs_mmap(bitvec_path, bitvec_info_path);
    ASSERT_EQ(indexvecs_mmap.info.sizes().size(),
              bitvecs_mmap.info.sizes().size());
    ASSERT_EQ(indexvecs_mmap.info.sizes().size(), ranges.size());

    const auto loaded_indexvecs =
        IndexVectorsIO::load_from_file(indexvecs_mmap);
    const auto loaded_bitvecs = BitVectorsIO::load_from_file(bitvecs_mmap);

    ASSERT_EQ(loaded_bitvecs.size(), loaded_indexvecs.size());
    ASSERT_EQ(loaded_bitvecs.size(), ranges.size());

    const auto &column_const = column;
    const auto column_view = column_const.view();
    index_vector indexvec;
    for (size_t i = 0; i < ranges.size(); ++i) {
      indexvec.resize(column.size());
      bit_vector bitvec(column.size());
      pnm::imdb::internal::avx2::scan(column_view, ranges[i], bitvec.view());
      const auto indexvec_size = pnm::imdb::internal::avx2::scan(
          column_view, ranges[i], pnm::make_view(indexvec));
      indexvec.resize(indexvec_size);

      ASSERT_EQ(indexvec, loaded_indexvecs[i]);
      ASSERT_EQ(bitvec, loaded_bitvecs[i]);
    }
  }
}

TEST_F(ImdbGoldenStoreLoad, GoldenTestPredicateVectors) {
  // first iteration is for case when generated data doesn't exist
  // second iteration tests reading already existing data
  for (int iteration = 0; iteration < 2; ++iteration) {
    GeneratorPredicateVectorsParams predictors_params;
    predictors_params.common_params_ = CommonGeneratorParams{
        .root_ = root_path,
        .filename_prefix_ = prefix,
        .generator_name_ = literals::predictor_generator_random};
    predictors_params.seed_ = 345;
    predictors_params.selectivities_ = std::vector<double>{0.1, 0.3, 0.5, 0.7};
    predictors_params.predicate_max_values_ =
        std::vector<compressed_element_type>{100'000, (1UL << 18) - 1, 10'000,
                                             200'000};

    const auto predictors =
        get_or_create_test_predicate_vectors(predictors_params);
    ASSERT_EQ(predictors.size(), predictors_params.selectivities_.size());

    const PredicateVectorsMmap predictors_mmap =
        get_mmap_predicate_vectors(root_path, prefix);
    ASSERT_EQ(predictors_mmap.info.selectivities(),
              predictors_params.selectivities_);
    ASSERT_EQ(predictors_mmap.info.predicate_max_values(),
              predictors_params.predicate_max_values_);

    const auto loaded_predictors =
        PredicateVectorsIO::load_from_file(predictors_mmap);
    ASSERT_EQ(loaded_predictors, predictors);

    const VectorOfVectorsMmap indexvecs_mmap(indexvec_path, indexvec_info_path);
    const VectorOfVectorsMmap bitvecs_mmap(bitvec_path, bitvec_info_path);
    ASSERT_EQ(indexvecs_mmap.info.sizes().size(),
              bitvecs_mmap.info.sizes().size());
    ASSERT_EQ(indexvecs_mmap.info.sizes().size(), predictors.size());

    const auto loaded_indexvecs =
        IndexVectorsIO::load_from_file(indexvecs_mmap);
    const auto loaded_bitvecs = BitVectorsIO::load_from_file(bitvecs_mmap);

    ASSERT_EQ(loaded_bitvecs.size(), loaded_indexvecs.size());
    ASSERT_EQ(loaded_bitvecs.size(), predictors.size());

    const auto &column_const = column;
    const auto column_view = column_const.view();
    index_vector indexvec;
    for (size_t i = 0; i < predictors.size(); ++i) {
      indexvec.resize(column.size());
      const auto predictor_view = predictors[i].view();
      bit_vector bitvec(column.size());
      pnm::imdb::internal::avx2::scan(column_view, predictor_view,
                                      bitvec.view());
      const auto indexvec_size = pnm::imdb::internal::avx2::scan(
          column_view, predictor_view, pnm::make_view(indexvec));
      indexvec.resize(indexvec_size);

      ASSERT_EQ(indexvec, loaded_indexvecs[i]);
      ASSERT_EQ(bitvec, loaded_bitvecs[i]);
    }
  }
}

template <OperationType input_type, OutputType output_type>
void test_default_generator() {
  const auto [column, input, result] = generate_data<input_type, output_type>();
  using Out = OutputDataType<output_type>;
  ASSERT_EQ(result, ImdbGoldenVecGenerator::scan<Out>(column, input));
}

TEST(ImdbGoldenTests, DefaultGenerators) {
  test_default_generator<OperationType::InRange, OutputType::IndexVector>();
  test_default_generator<OperationType::InRange, OutputType::BitVector>();
  test_default_generator<OperationType::InList, OutputType::IndexVector>();
  test_default_generator<OperationType::InList, OutputType::BitVector>();
}
