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

#ifndef IMDB_TRIVIAL_MAX_VALUES_GENERATOR_H
#define IMDB_TRIVIAL_MAX_VALUES_GENERATOR_H

#include "base.h"

#include "pnmlib/imdb/scan_types.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace tools::gen::imdb {

using namespace pnm::imdb;

/** @brief Template for trivial max value generator with variable
 *   generation strategy
 */
template <typename EntryGen>
class TrivialMaxValuesGenerator : public IMaxValuesGenerator {
public:
  TrivialMaxValuesGenerator() = default;

  explicit TrivialMaxValuesGenerator(EntryGen entry_generator)
      : entry_generator_{std::move(entry_generator)} {}

private:
  std::vector<compressed_element_type> create_max_values_impl(
      size_t num_values, [[maybe_unused]] uint64_t entry_bit_size,
      [[maybe_unused]] compressed_element_type min_value,
      [[maybe_unused]] compressed_element_type max_value) override {
    std::vector<compressed_element_type> values;
    values.resize(num_values);
    for (size_t index = 0UL; index < num_values; ++index) {
      values[index] = entry_generator_(index);
    }

    return values;
  }

  EntryGen entry_generator_;
};

} // namespace tools::gen::imdb

#endif // IMDB_TRIVIAL_MAX_VALUES_GENERATOR_H
