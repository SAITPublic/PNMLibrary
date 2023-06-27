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

#include "pnmlib/imdb/bit_containers.h"
#include "pnmlib/imdb/bit_views.h"
#include "pnmlib/imdb/scan_types.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <fmt/core.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>

using namespace pnm::imdb;
using namespace tools::gen::imdb;

namespace {

bool are_equal(const compressed_vector &lhs, const compressed_vector &rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  if (lhs.value_bits() != rhs.value_bits()) {
    return false;
  }

  for (size_t i = 0UL; i < lhs.size(); ++i) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }

  return true;
}
} // namespace

class GenColumnTest
    : public testing::TestWithParam<std::tuple<std::string, size_t, uint64_t>> {
public:
  void run() {
    const auto [gen_name, entry_count, entry_bit_size] = GetParam();

    const auto &factory = ColumnGeneratorFactory::default_factory();
    const auto entry_max_value = (1UL << entry_bit_size) - 1;

    const ColumnsInfo info(entry_bit_size, entry_count);

    auto generator = factory.create(gen_name, std::to_string(entry_bit_size));
    const auto column = generator->create(info);
    ASSERT_EQ(column.size(), entry_count);

    for (size_t i = 0; i < entry_count; ++i) {
      ASSERT_LE(column[i], entry_max_value);
    }
  }
};

class GenRandomColumnTest
    : public testing::TestWithParam<std::tuple<size_t, uint64_t>> {
public:
  void run() {
    const auto [entry_count, entry_bit_size] = GetParam();
    const ColumnsInfo info(entry_bit_size, entry_count);

    const std::string gen_name = literals::column_random_generator;
    const std::string factory_arg_no_seed = std::to_string(entry_bit_size);
    const auto template_arg = factory_arg_no_seed + " {}";

    const auto &factory = ColumnGeneratorFactory::default_factory();
    auto create_column = [&factory, &gen_name,
                          &info](const std::string &factory_arg) {
      auto gen = factory.create(gen_name, factory_arg);
      return gen->create(info);
    };
    const auto column_seed_1 =
        create_column(fmt::format(fmt::runtime(template_arg), 234));
    const auto column_seed_2 =
        create_column(fmt::format(fmt::runtime(template_arg), 234));
    const auto column_seed_3 =
        create_column(fmt::format(fmt::runtime(template_arg), 345));
    const auto column_no_seed_1 = create_column(factory_arg_no_seed);
    const auto column_no_seed_2 = create_column(factory_arg_no_seed);

    ASSERT_TRUE(are_equal(column_seed_1, column_seed_2));
    ASSERT_FALSE(are_equal(column_seed_1, column_seed_3));
    EXPECT_FALSE(are_equal(column_no_seed_1, column_no_seed_2));
    EXPECT_FALSE(are_equal(column_no_seed_1, column_seed_1));
  }
};

class PositionedColumnTest
    : public testing::TestWithParam<std::tuple<size_t, uint64_t, int32_t>> {
public:
  void run() {
    const auto [entry_count, entry_bit_size, step] = GetParam();
    const std::string factory_args = fmt::format("{} {}", entry_bit_size, step);
    const auto &factory = ColumnGeneratorFactory::default_factory();
    const int64_t entry_max_value = (1 << entry_bit_size) - 1;

    const ColumnsInfo info(entry_bit_size, entry_count);
    auto generator =
        factory.create(literals::column_positioned_generator, factory_args);
    const auto column = generator->create(info);
    for (size_t i = 1UL; i < column.size(); ++i) {
      const auto value_prev = static_cast<int64_t>(column[i - 1]);
      const auto value_next = static_cast<int64_t>(column[i]);
      const int64_t expected_next_value = value_prev + step;
      if (expected_next_value <= entry_max_value && expected_next_value >= 0) {
        ASSERT_EQ(expected_next_value, value_next);
      }
    }
  }
};

TEST_P(GenColumnTest, ImdbGenerators) { this->run(); }
TEST_P(GenRandomColumnTest, ImdbGenerators) { this->run(); }
TEST_P(PositionedColumnTest, ImdbGenerators) { this->run(); }

auto common_column_testing_parameters = testing::Combine(
    // generator_name
    testing::Values(std::string{literals::column_random_generator},
                    std::string{literals::column_positioned_generator}),
    // entry_count
    testing::Values(100),
    // entry_bit_size
    testing::Values(1, 2, 3, 4, 5, 6, 7, 9, 10, 12, 16, 17, 18));

auto random_column_testing_parameters = testing::Combine(
    // entry_count
    testing::Values(10),
    // entry_bit_size
    testing::Values(18));

auto positioned_column_testing_parameters = testing::Combine(
    // entry_count
    testing::Values(10000),
    // entry_bit_size
    testing::Values(1, 18),
    // step
    testing::Values(-1'000'000'000, -10'000, -222, -33, -1, 0, 1, 333, 222,
                    10'000, 1'000'000'000));

INSTANTIATE_TEST_SUITE_P(GenColumn, GenColumnTest,
                         common_column_testing_parameters);
INSTANTIATE_TEST_SUITE_P(GenColumn, GenRandomColumnTest,
                         random_column_testing_parameters);
INSTANTIATE_TEST_SUITE_P(GenColumn, PositionedColumnTest,
                         positioned_column_testing_parameters);
