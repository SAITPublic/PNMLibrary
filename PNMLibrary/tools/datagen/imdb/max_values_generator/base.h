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

#ifndef IMDB_IMAX_VALUES_GENERATOR_H
#define IMDB_IMAX_VALUES_GENERATOR_H

#include "pnmlib/imdb/scan_types.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace tools::gen::imdb {

using namespace pnm::imdb;

/** @brief Interface for predicate vector's max value generators */
class IMaxValuesGenerator {
public:
  auto create(size_t num_values, uint64_t entry_bit_size,
              compressed_element_type min_value,
              compressed_element_type max_value) {
    return create_max_values_impl(num_values, entry_bit_size, min_value,
                                  max_value);
  }

  virtual ~IMaxValuesGenerator() = default;

private:
  virtual std::vector<compressed_element_type>
  create_max_values_impl(size_t num_values, uint64_t entry_bit_size,
                         compressed_element_type min_value,
                         compressed_element_type max_value) = 0;
};

} // namespace tools::gen::imdb

#endif // IMDB_IMAX_VALUES_GENERATOR_H
