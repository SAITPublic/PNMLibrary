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

#include "factory.h"

#include "duplicate_random.h"
#include "random.h"

#include <memory>
#include <sstream>

namespace tools::gen::sls {

LengthsGeneratorFactory &LengthsGeneratorFactory::default_factory() {
  static LengthsGeneratorFactory factory;

  factory.register_builder("random", [](const auto &args) {
    std::istringstream iss{args};
    int overflow_type;
    iss >> overflow_type;
    return std::make_unique<RandomLengthsGen>(overflow_type);
  });

  factory.register_builder(
      "random_from_lookups_per_table", []([[maybe_unused]] const auto &args) {
        return std::make_unique<DuplicateRandomLengthsGen>();
      });

  return factory;
}

} // namespace tools::gen::sls
