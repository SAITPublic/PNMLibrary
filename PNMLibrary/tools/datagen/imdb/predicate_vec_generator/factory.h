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

#ifndef IMDB_PREDICATE_VECS_GEN_FACTORY_H
#define IMDB_PREDICATE_VECS_GEN_FACTORY_H

#include "base.h"

#include "tools/datagen/common_factory.h"

#include "pnmlib/imdb/scan_types.h"

#include <memory>
#include <optional>
#include <string>

namespace tools::gen::imdb {

namespace literals {
constexpr inline auto predictor_generator_random = "random";
constexpr inline auto predictor_generator_random_precise = "random_precise";
} // namespace literals
/** @brief Factory for predicate vector generators. Call default_factory() to
 * get factory with preregistered generators.
 */
class PredicateVecsGeneratorFactory
    : public CommonFactory<
          std::string, std::unique_ptr<IPredicateVecsGenerator>,
          const std::string &,
          std::optional<std::shared_ptr<const compressed_vector>>> {
public:
  static PredicateVecsGeneratorFactory &default_factory();
};

} // namespace tools::gen::imdb

#endif // IMDB_PREDICATE_VECS_GEN_FACTORY_H
