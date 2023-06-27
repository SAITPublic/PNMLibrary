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

#include "default_entry_gens.h"
#include "trivial.h"

#include <cstdint>
#include <memory>
#include <sstream>

TablesGeneratorFactory &TablesGeneratorFactory::default_factory() {
  static TablesGeneratorFactory factory;
  // Register your builder here by calling fabric.register_builder(....)
  factory.register_builder("random", []([[maybe_unused]] const auto &args) {
    return std::make_unique<TrivialTablesGenerator<uint32_t, RandomEntry>>();
  });

  factory.register_builder("position", []([[maybe_unused]] const auto &args) {
    return std::make_unique<
        TrivialTablesGenerator<uint32_t, PositionedEntry>>();
  });

  factory.register_builder("float_tapp", [](const auto &args) {
    std::istringstream iss{args};
    uint32_t kvalue, sparse_feature_size;
    iss >> kvalue >> sparse_feature_size;
    return std::make_unique<TrivialTablesGenerator<float, NumericEntry<float>>>(
        kvalue, sparse_feature_size);
  });

  factory.register_builder("uint32_t_tapp", [](const auto &args) {
    std::istringstream iss{args};
    uint32_t kvalue, sparse_feature_size;
    iss >> kvalue >> sparse_feature_size;
    return std::make_unique<
        TrivialTablesGenerator<uint32_t, NumericEntry<uint32_t>>>(
        kvalue, sparse_feature_size);
  });

  factory.register_builder("fixed", [](const auto &args) {
    std::istringstream iss{args};
    uint32_t value;
    iss >> value;
    return std::make_unique<TrivialTablesGenerator<uint32_t, FixedEntry>>(
        value);
  });

  return factory;
}
