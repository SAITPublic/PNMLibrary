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

#include "trivial.h"

#include "tools/datagen/imdb/general/column_entry_gens.h"
#include "tools/datagen/imdb/general/column_entry_gens_creator.h"

#include <memory>

namespace tools::gen::imdb {

MaxValuesGeneratorFactory &MaxValuesGeneratorFactory::default_factory() {
  static MaxValuesGeneratorFactory factory = []() {
    MaxValuesGeneratorFactory factory;
    // Register your builder here by calling register_builder(....)
    factory.register_builder(
        literals::max_values_random_generator, [](const auto &args) {
          const RandomEntry entry_gen =
              create_entry_generator<RandomEntry>(args);
          return std::make_unique<TrivialMaxValuesGenerator<RandomEntry>>(
              entry_gen);
        });
    factory.register_builder(
        literals::max_values_positioned_generator, [](const auto &args) {
          const PositionedEntry entry_gen =
              create_entry_generator<PositionedEntry>(args);
          return std::make_unique<TrivialMaxValuesGenerator<PositionedEntry>>(
              entry_gen);
        });
    return factory;
  }();
  return factory;
}

} // namespace tools::gen::imdb
