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

#ifndef IMDB_TABLE_COLUMN_GENERATOR_FACTORY_H
#define IMDB_TABLE_COLUMN_GENERATOR_FACTORY_H

#include "base.h"

#include "tools/datagen/common_factory.h"

#include <memory>
#include <string>

namespace tools::gen::imdb {

namespace literals {
inline constexpr auto column_random_generator = "random";
inline constexpr auto column_positioned_generator = "positioned";
} // namespace literals

/** @brief Factory for compressed column generators. Call
 *   default_factory() method to get Factory with all preregistered generators.
 */
class ColumnGeneratorFactory
    : public CommonFactory<std::string, std::unique_ptr<IColumnGenerator>,
                           const std::string &> {
public:
  static ColumnGeneratorFactory &default_factory();
};

} // namespace tools::gen::imdb

#endif // IMDB_TABLE_COLUMN_GENERATOR_FACTORY_H
