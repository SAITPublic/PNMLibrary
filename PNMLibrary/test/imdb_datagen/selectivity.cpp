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

#include "tools/datagen/imdb/selectivities_generator/base.h"
#include "tools/datagen/imdb/selectivities_generator/factory.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <tuple>
#include <vector>

using namespace tools::gen::imdb;

class GenSelectivityTest
    : public testing::TestWithParam<
          std::tuple<std::string, size_t, double, double>> {
public:
  void run() {
    const auto [gen_name, count, min_selectivity, max_selectivity] = GetParam();
    const auto factory = SelectivitiesGeneratorFactory::default_factory();
    auto generator = factory.create(gen_name);
    std::vector<double> selectivities;
    const bool invalid_input = min_selectivity > max_selectivity ||
                               min_selectivity < 0. || max_selectivity < 0. ||
                               max_selectivity > 1. || min_selectivity > 1.;

    if (invalid_input) {
      ASSERT_ANY_THROW(selectivities = generator->create(count, min_selectivity,
                                                         max_selectivity););
      return;
    }

    selectivities = generator->create(count, min_selectivity, max_selectivity);

    auto valid_condition = [min_selectivity = min_selectivity,
                            max_selectivity =
                                max_selectivity](double selectivity) -> bool {
      return selectivity <= max_selectivity && selectivity >= min_selectivity;
    };

    ASSERT_EQ(selectivities.size(), count);
    ASSERT_TRUE(std::all_of(selectivities.begin(), selectivities.end(),
                            valid_condition));
  }
};

class GenRandomSelectivityTest
    : public testing::TestWithParam<std::tuple<size_t, double, double>> {
public:
  void run() {
    const auto selec_creator = [](ISelectivitiesGenerator *gen) {
      const auto [count, min_selectivity, max_selectivity] =
          GenRandomSelectivityTest::GetParam();
      return gen->create(count, min_selectivity, max_selectivity);
    };

    const auto factory = SelectivitiesGeneratorFactory::default_factory();
    const std::string gen_name = literals::selectivities_random_gen;
    auto gen_seed_1 = factory.create(gen_name, "234");
    auto gen_seed_2 = factory.create(gen_name, "234");
    auto gen_seed_3 = factory.create(gen_name, "345");
    auto gen_no_seed_1 = factory.create(gen_name);
    auto gen_no_seed_2 = factory.create(gen_name);

    const auto selectivity_seed_1 = selec_creator(gen_seed_1.get());
    const auto selectivity_seed_2 = selec_creator(gen_seed_2.get());
    const auto selectivity_seed_3 = selec_creator(gen_seed_3.get());
    const auto selectivity_no_seed_1 = selec_creator(gen_no_seed_1.get());
    const auto selectivity_no_seed_2 = selec_creator(gen_no_seed_2.get());

    ASSERT_EQ(selectivity_seed_1, selectivity_seed_2);
    ASSERT_NE(selectivity_seed_1, selectivity_seed_3);
    EXPECT_NE(selectivity_no_seed_1, selectivity_no_seed_2);
    EXPECT_NE(selectivity_no_seed_1, selectivity_seed_1);
  }
};

TEST_P(GenSelectivityTest, ImdbGenerators) { this->run(); }
TEST_P(GenRandomSelectivityTest, ImdbGenerators) { this->run(); }

auto common_selectivity_testing_parameters = testing::Combine(
    // generator_name
    testing::Values(std::string{literals::selectivities_random_gen},
                    std::string{literals::selectivities_sequential_gen}),
    // count
    testing::Values(100),
    // min_selectivity
    testing::Values(-1., 0., 0.3, 1., 1.2),
    // max_selectivity
    testing::Values(-1., 0., 0.3, 1., 1.2));

auto random_selectivity_testing_parameters =
    // count
    testing::Combine(testing::Values(10),
                     // min_selectivity
                     testing::Values(0.2),
                     // max_selectivity
                     testing::Values(0.6));

INSTANTIATE_TEST_SUITE_P(GenSelectivity, GenSelectivityTest,
                         common_selectivity_testing_parameters);
INSTANTIATE_TEST_SUITE_P(GenSelectivity, GenRandomSelectivityTest,
                         random_selectivity_testing_parameters);
