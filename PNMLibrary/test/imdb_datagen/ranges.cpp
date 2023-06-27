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
#include "tools/datagen/imdb/general/ranges_info.h"
#include "tools/datagen/imdb/range_generator/base.h"
#include "tools/datagen/imdb/range_generator/factory.h"

#include "pnmlib/imdb/scan_types.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <fmt/core.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

using namespace pnm::imdb;
using namespace tools::gen::imdb;

constexpr double precise_epsilon_selectivity = 0.01;
// might fail if input test data is changed
constexpr double standard_epsilon_selectivity = 0.1;

class GenScanRangeTest
    : public testing::TestWithParam<std::tuple<
          uint64_t, size_t, size_t, std::string, size_t, std::vector<double>>> {
public:
  void run() {
    const auto [entry_bit_size, entry_count, column_seed, gen_name, seed,
                selectivities] = GetParam();
    const ColumnsInfo column_info(entry_bit_size, entry_count);
    const auto &column_factory = ColumnGeneratorFactory::default_factory();
    auto column_generator = column_factory.create(
        literals::column_random_generator,
        fmt::format("{} {}", column_info.entry_bit_size(), column_seed));
    const auto column = std::make_shared<const compressed_vector>(
        column_generator->create(column_info));
    ASSERT_NE(column->size(), 0);

    const RangesInfo info(selectivities);

    const auto &factory = RangesGeneratorFactory::default_factory();
    std::unique_ptr<IRangesGenerator> generator;
    double selectivity_epsilon;

    if (gen_name == literals::scan_ranges_generator_random) {
      selectivity_epsilon = standard_epsilon_selectivity;
      generator = factory.create(gen_name, std::to_string(seed), std::nullopt);
    } else if (gen_name == literals::scan_ranges_generator_random_precise) {
      selectivity_epsilon = precise_epsilon_selectivity;
      generator = factory.create(
          gen_name, fmt::format("{} {}", seed, precise_epsilon_selectivity),
          column);
    } else {
      FAIL() << "Scan ranges generator type not supported";
    }

    auto scan_ranges = generator->create(info, column_info);
    ASSERT_EQ(scan_ranges.size(), info.num_requests());
    ASSERT_EQ(scan_ranges.size(), info.selectivities().size());

    const auto entry_max_value = (1UL << column_info.entry_bit_size()) - 1;

    for (size_t i = 0; i < scan_ranges.size(); ++i) {
      const auto &range = scan_ranges[i];
      const auto expected_selectivity = info.selectivities()[i];

      ASSERT_LE(range.start, range.end);
      ASSERT_LE(range.end, entry_max_value);
      ASSERT_LE(range.start, entry_max_value);

      const auto actual_selectivity = calc_actual_selectivity(*column, range);
      EXPECT_NEAR(actual_selectivity, expected_selectivity,
                  selectivity_epsilon);
    }
  }
};

TEST_P(GenScanRangeTest, ImdbGenerators) { this->run(); }

auto scan_ranges_testing_parameters = testing::Combine(
    // entry_bit_size
    testing::Values(18),
    // entry_count
    testing::Values(1'000, 1'000'000),
    // column seed
    testing::Values(678),
    // generator name
    testing::Values(
        std::string{literals::scan_ranges_generator_random},
        std::string{literals::scan_ranges_generator_random_precise}),
    // seed
    testing::Values(123, 234, 345),
    // selectivities
    testing::Values(std::vector<double>{0.1, 0.2, 0.3, 0.4},
                    std::vector<double>{0.5, 0.6, 0.7, 0.8}));

INSTANTIATE_TEST_SUITE_P(GenScanRanges, GenScanRangeTest,
                         scan_ranges_testing_parameters);
