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

#ifndef IMDB_SCAN_RANGES_GEN_FACTORY_H
#define IMDB_SCAN_RANGES_GEN_FACTORY_H

#include "base.h"

#include "tools/datagen/common_factory.h"

#include "pnmlib/imdb/scan_types.h"

#include <memory>
#include <optional>
#include <string>

namespace tools::gen::imdb {

namespace literals {
inline constexpr auto scan_ranges_generator_random = "random";
inline constexpr auto scan_ranges_generator_random_precise = "random_precise";
} // namespace literals

/** @brief Factory for scan ranges generators. Call default_factory() to get
 * factory with preregistered generators.
 */
class RangesGeneratorFactory
    : public CommonFactory<
          std::string, std::unique_ptr<IRangesGenerator>, const std::string &,
          std::optional<std::shared_ptr<const compressed_vector>>> {
public:
  static RangesGeneratorFactory &default_factory();
};
} // namespace tools::gen::imdb

#endif // IMDB_SCAN_RANGES_GEN_FACTORY_H
