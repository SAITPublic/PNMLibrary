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

#include "random.h"
#include "seq.h"

#include <cstddef>
#include <memory>
#include <sstream>

namespace tools::gen::imdb {
SelectivitiesGeneratorFactory &
SelectivitiesGeneratorFactory::default_factory() {
  static auto factory = []() {
    static SelectivitiesGeneratorFactory factory;
    factory.register_builder(
        literals::selectivities_random_gen, [](const auto &args) {
          std::istringstream iss{args};
          size_t seed;
          iss >> seed;
          if (!iss) {
            return std::make_unique<RandomSelectivitiesGen>();
          }
          return std::make_unique<RandomSelectivitiesGen>(seed);
        });

    factory.register_builder(literals::selectivities_sequential_gen,
                             []([[maybe_unused]] const auto &args) {
                               return std::make_unique<SeqSelectivitiesGen>();
                             });
    return factory;
  }();
  return factory;
}

} // namespace tools::gen::imdb
