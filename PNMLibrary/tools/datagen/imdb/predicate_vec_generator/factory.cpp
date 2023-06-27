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

#include "factory.h"

#include "precise_random.h"
#include "random.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace tools::gen::imdb {

PredicateVecsGeneratorFactory &
PredicateVecsGeneratorFactory::default_factory() {
  static PredicateVecsGeneratorFactory factory = []() {
    PredicateVecsGeneratorFactory factory;
    factory.register_builder(
        literals::predictor_generator_random,
        [](const auto &args,
           [[maybe_unused]] const auto &column = std::nullopt) {
          std::istringstream iss{args};
          size_t seed;
          iss >> seed;
          if (!iss) {
            return std::make_unique<RandomPredicateVecsGen>();
          }
          return std::make_unique<RandomPredicateVecsGen>(seed);
        });

    factory.register_builder(
        literals::predictor_generator_random_precise,
        [](const auto &args, const auto &column) {
          std::istringstream iss{args};
          size_t seed;
          iss >> seed;
          if (!column.has_value()) {
            throw std::invalid_argument(
                "RangesGeneratorFactory "
                "must contain compressed column as parameter");
          }
          if (!iss) {
            return std::make_unique<PreciseRandomPredicateVecsGen>(
                column.value());
          }
          double selectivity_epsilon_;
          iss >> selectivity_epsilon_;
          if (!iss) {
            return std::make_unique<PreciseRandomPredicateVecsGen>(
                column.value(), seed);
          }
          return std::make_unique<PreciseRandomPredicateVecsGen>(
              column.value(), seed, selectivity_epsilon_);
        });
    return factory;
  }();
  return factory;
}

} // namespace tools::gen::imdb
