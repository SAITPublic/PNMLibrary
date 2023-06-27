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

#ifndef IMDB_MAX_VALUES_GEN_FACTORY_H
#define IMDB_MAX_VALUES_GEN_FACTORY_H

#include "base.h"

#include "tools/datagen/common_factory.h"

#include <memory>
#include <string>

namespace tools::gen::imdb {

using namespace pnm::imdb;

namespace literals {
inline constexpr auto max_values_random_generator = "random";
inline constexpr auto max_values_positioned_generator = "positioned";
} // namespace literals

/** @brief Factory for max values of predicate vectors generators. Call
 *  default_factory() to get factory with preregistered generators.
 */
class MaxValuesGeneratorFactory
    : public CommonFactory<std::string, std::unique_ptr<IMaxValuesGenerator>,
                           const std::string &> {
public:
  static MaxValuesGeneratorFactory &default_factory();
};

} // namespace tools::gen::imdb

#endif // IMDB_MAX_VALUES_GEN_FACTORY_H
