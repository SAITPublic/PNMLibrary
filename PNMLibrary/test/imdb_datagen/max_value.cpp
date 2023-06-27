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

#include "tools/datagen/imdb/max_values_generator/base.h"
#include "tools/datagen/imdb/max_values_generator/factory.h"

#include "pnmlib/imdb/scan_types.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <fmt/core.h>

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <tuple>
#include <vector>

using namespace pnm::imdb;
using namespace tools::gen::imdb;

class GenPredicateMaxValueTest
    : public testing::TestWithParam<
          std::tuple<std::string, size_t, uint64_t, compressed_element_type,
                     compressed_element_type>> {
public:
  void run() {
    const auto [gen_name, count, entry_bit_size, min_value, max_value] =
        GetParam();

    const auto template_args_builder = [entry_bit_size = entry_bit_size,
                                        min_value = min_value,
                                        max_value = max_value](auto arg) {
      return fmt::format("{} {} {} {}", entry_bit_size, arg, min_value,
                         max_value);
    };

    std::string factory_args;
    if (gen_name == literals::max_values_random_generator) {
      const auto seed = std::random_device{}();
      factory_args = template_args_builder(seed);
    } else if (gen_name == literals::max_values_positioned_generator) {
      constexpr auto step = 1;
      factory_args = template_args_builder(step);
    } else {
      FAIL() << "Entry generator type no supported";
    }

    const auto &factory = MaxValuesGeneratorFactory::default_factory();
    const auto entry_max_value = (1UL << entry_bit_size) - 1;
    const bool invalid_input = min_value > max_value ||
                               min_value > entry_max_value ||
                               max_value > entry_max_value;

    if (invalid_input) {
      ASSERT_ANY_THROW(auto generator =
                           factory.create(gen_name, factory_args););
      return;
    }

    auto generator = factory.create(gen_name, factory_args);
    const auto predicate_max_values =
        generator->create(count, entry_bit_size, min_value, max_value);
    ASSERT_EQ(predicate_max_values.size(), count);
    for (auto value : predicate_max_values) {
      ASSERT_LE(value, entry_max_value);
      ASSERT_LE(value, max_value);
      ASSERT_LE(min_value, value);
    }
  }
};

class GenRandomPredicateMaxValueTest
    : public testing::TestWithParam<std::tuple<
          size_t, uint64_t, compressed_element_type, compressed_element_type>> {
public:
  void run() {
    const auto &factory = MaxValuesGeneratorFactory::default_factory();
    const auto create_values = [&factory](const auto seed) {
      const auto [count, entry_bit_size, min_value, max_value] =
          GenRandomPredicateMaxValueTest::GetParam();
      const auto args = fmt::format("{} {} {} {}", entry_bit_size, seed,
                                    min_value, max_value);
      auto gen = factory.create(literals::max_values_random_generator, args);
      return gen->create(count, entry_bit_size, min_value, max_value);
    };

    auto values_seed_1 = create_values(234);
    auto values_seed_2 = create_values(234);
    auto values_seed_3 = create_values(345);

    ASSERT_EQ(values_seed_1, values_seed_2);
    ASSERT_NE(values_seed_1, values_seed_3);
  }
};

class GenPositionedPredicateMaxValueTest
    : public testing::TestWithParam<
          std::tuple<size_t, uint64_t, compressed_element_type,
                     compressed_element_type, int32_t>> {
public:
  void run() {
    const auto [count, entry_bit_size, min_value, max_value, step] = GetParam();
    const std::string factory_args =
        fmt::format("{} {} {} {}", entry_bit_size, step, min_value, max_value);
    const auto &factory = MaxValuesGeneratorFactory::default_factory();
    const std::string gen_name = literals::max_values_positioned_generator;

    const auto entry_max_value = (1UL << entry_bit_size) - 1;
    const bool invalid_input = min_value > max_value ||
                               min_value > entry_max_value ||
                               max_value > entry_max_value;

    if (invalid_input) {
      ASSERT_ANY_THROW(auto generator =
                           factory.create(gen_name, factory_args););
      return;
    }

    auto generator = factory.create(gen_name, factory_args);
    const auto predicate_max_values =
        generator->create(count, entry_bit_size, min_value, max_value);
    for (size_t i = 1UL; i < predicate_max_values.size(); ++i) {
      const auto value_prev = static_cast<int64_t>(predicate_max_values[i - 1]);
      const auto value_next = static_cast<int64_t>(predicate_max_values[i]);
      const int64_t expected_next_value = value_prev + step;
      if (expected_next_value >= min_value &&
          expected_next_value <= max_value) {
        ASSERT_EQ(expected_next_value, value_next);
      }
    }
  }
};

TEST_P(GenPredicateMaxValueTest, ImdbGenerators) { this->run(); }
TEST_P(GenRandomPredicateMaxValueTest, ImdbGenerators) { this->run(); }
TEST_P(GenPositionedPredicateMaxValueTest, ImdbGenerators) { this->run(); }

auto common_max_values_testing_parameters = testing::Combine(
    // generator name
    testing::Values(std::string{literals::max_values_random_generator},
                    std::string{literals::max_values_positioned_generator}),
    // count
    testing::Values(50),
    // entry_bit_size
    testing::Values(1, 4, 10, 18),
    // min_value
    testing::Values(0, 1, 1'000, 100'000, 1'000'000'000),
    // max_value
    testing::Values(0, 1, 1'000, 100'000, 1'000'000'000));

auto random_max_values_testing_parameters = testing::Combine(
    // count
    testing::Values(10),
    // entry_bit_size
    testing::Values(18),
    // min_value
    testing::Values(0),
    // max_value
    testing::Values(100));

auto positioned_max_values_testing_parameters = testing::Combine(
    // count
    testing::Values(10000),
    // entry_bit_size
    testing::Values(1, 18),
    // min_value
    testing::Values(0, 10, 100),
    // max_value
    testing::Values(1'000, 10'000, 100'000),
    // step
    testing::Values(-1'000'000'000, -10'000, -222, -33, -1, 0, 1, 333, 222,
                    10'000, 1'000'000'000));

INSTANTIATE_TEST_SUITE_P(GenPredicateMaxValue, GenPredicateMaxValueTest,
                         common_max_values_testing_parameters);
INSTANTIATE_TEST_SUITE_P(GenPredicateMaxValue, GenRandomPredicateMaxValueTest,
                         random_max_values_testing_parameters);
INSTANTIATE_TEST_SUITE_P(GenPredicateMaxValue,
                         GenPositionedPredicateMaxValueTest,
                         positioned_max_values_testing_parameters);
