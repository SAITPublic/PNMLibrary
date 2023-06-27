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

#include "tools/datagen/imdb/column_generator/base.h"
#include "tools/datagen/imdb/column_generator/factory.h"
#include "tools/datagen/imdb/general/columns_info.h"
#include "tools/datagen/imdb/general/generators_utils.h"
#include "tools/datagen/imdb/general/predicate_vecs_info.h"
#include "tools/datagen/imdb/predicate_vec_generator/base.h"
#include "tools/datagen/imdb/predicate_vec_generator/factory.h"

#include "pnmlib/imdb/bit_containers.h"
#include "pnmlib/imdb/scan_types.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <fmt/core.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

using namespace pnm::imdb;
using namespace tools::gen::imdb;

constexpr double standard_epsilon_selectivity =
    0.15; // can fail if input test data is changed
constexpr double precise_epsilon_selectivity = 0.001;

class GenPredictorsTest
    : public testing::TestWithParam<std::tuple<
          uint64_t, size_t, size_t, std::string, size_t, std::vector<double>,
          std::vector<compressed_element_type>>> {
public:
  void run() {
    const auto [entry_bit_size, entry_count, column_seed, gen_name, seed,
                selectivities, max_values] = GetParam();
    const ColumnsInfo column_info(entry_bit_size, entry_count);
    const auto &column_factory = ColumnGeneratorFactory::default_factory();
    auto column_generator = column_factory.create(
        literals::column_random_generator,
        fmt::format("{} {}", column_info.entry_bit_size(), column_seed));
    const auto column = std::make_shared<const compressed_vector>(
        column_generator->create(column_info));

    ASSERT_NE(column->size(), 0);

    const PredicateVectorsInfo info(selectivities, max_values);
    const auto entry_max_value = (1UL << column_info.entry_bit_size()) - 1;

    const auto &factory = PredicateVecsGeneratorFactory::default_factory();
    std::unique_ptr<IPredicateVecsGenerator> generator;
    double selectivity_epsilon = 1.;
    if (gen_name == literals::predictor_generator_random) {
      generator = factory.create(gen_name, std::to_string(seed), std::nullopt);
      selectivity_epsilon = standard_epsilon_selectivity;
    } else if (gen_name == literals::predictor_generator_random_precise) {
      generator = factory.create(
          gen_name, fmt::format("{} {}", seed, precise_epsilon_selectivity),
          column);
      selectivity_epsilon = precise_epsilon_selectivity;
    } else {
      FAIL() << "Predicate vectors generator type not supported";
    }

    auto predictors = generator->create(info, column_info);
    ASSERT_EQ(predictors.size(), info.num_requests());
    ASSERT_EQ(predictors.size(), info.selectivities().size());
    ASSERT_EQ(predictors.size(), info.predicate_max_values().size());

    // cached maximal selectivities for max values
    std::map<compressed_element_type, double> max_value_to_max_selectivity;

    for (size_t i = 0; i < predictors.size(); ++i) {
      const auto &predictor = predictors[i];
      const auto expected_selectivity = info.selectivities()[i];
      const auto expected_max_value = info.predicate_max_values()[i];
      ASSERT_EQ(predictor.size(), expected_max_value + 1);
      ASSERT_LE(predictor.size(), entry_max_value + 1);
      const auto actual_selectivity =
          calc_actual_selectivity(*column, predictor);

      if (expected_max_value == entry_max_value) {
        EXPECT_NEAR(actual_selectivity, expected_selectivity,
                    selectivity_epsilon);
      } else if (gen_name == literals::predictor_generator_random_precise) {
        double max_selectivity;
        if (auto it = max_value_to_max_selectivity.find(expected_max_value);
            it != max_value_to_max_selectivity.end()) {
          max_selectivity = it->second;
        } else {
          max_selectivity = max_value_to_max_selectivity[expected_max_value] =
              calc_actual_selectivity(*column,
                                      RangeOperation{0, expected_max_value});
        }

        EXPECT_NEAR(actual_selectivity,
                    std::min(expected_selectivity, max_selectivity),
                    selectivity_epsilon);
      }
    }
  }
};

TEST_P(GenPredictorsTest, ImdbGenerators) { this->run(); }

auto predictors_testing_parameters = testing::Combine(
    // entry_bit_size
    testing::Values(18),
    // entry_count
    testing::Values(1'000'000),
    // column seed
    testing::Values(678),
    // generator name
    testing::Values(std::string{literals::predictor_generator_random},
                    std::string{literals::predictor_generator_random_precise}),
    // seed
    testing::Values(123, 234, 345),
    // selectivities
    testing::Values(std::vector<double>{0.05, 0.1, 0.2, 0.3, 0.4, 0.45, 0.5,
                                        0.6, 0.7, 0.8, 0.9, 0.95}),
    // predicate vector max values
    testing::Values(std::vector<compressed_element_type>(12, 100'000),
                    std::vector<compressed_element_type>(12, (1U << 18) - 1)));

INSTANTIATE_TEST_SUITE_P(GenPredicateVectors, GenPredictorsTest,
                         predictors_testing_parameters);
