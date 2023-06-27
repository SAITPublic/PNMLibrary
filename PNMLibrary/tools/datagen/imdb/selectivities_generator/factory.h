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

#ifndef IMDB_SELECTIVITIES_GEN_FACTORY_H
#define IMDB_SELECTIVITIES_GEN_FACTORY_H

#include "base.h"

#include "tools/datagen/common_factory.h"

#include <memory>
#include <string>

namespace tools::gen::imdb {

namespace literals {
constexpr inline auto selectivities_random_gen = "random";
constexpr inline auto selectivities_sequential_gen = "sequential";
} // namespace literals

/**
  @brief Factory for selectivities of scan ranges and predicate vectors
  generators. Call default_factory() to get factory with preregistered
  generators.
*/
class SelectivitiesGeneratorFactory
    : public CommonFactory<std::string,
                           std::unique_ptr<ISelectivitiesGenerator>,
                           const std::string &> {
public:
  static SelectivitiesGeneratorFactory &default_factory();
};

} // namespace tools::gen::imdb

#endif // IMDB_SELECTIVITIES_GEN_FACTORY_H
