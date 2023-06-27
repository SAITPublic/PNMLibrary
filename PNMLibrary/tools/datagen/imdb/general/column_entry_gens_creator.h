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

#ifndef TABLE_COLUMN_ENTRY_GENS_CREATOR_H
#define TABLE_COLUMN_ENTRY_GENS_CREATOR_H

#include "column_entry_gens.h"

#include "pnmlib/imdb/scan_types.h"

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace tools::gen::imdb {

using namespace pnm::imdb;

namespace details {
template <typename T> struct always_false : std::false_type {};
} // namespace details

template <typename EntryGen>
inline EntryGen
create_entry_generator([[maybe_unused]] const std::string &args) {
  static_assert(details::always_false<EntryGen>::value,
                "EntryGen type is not supported");
}

template <>
inline RandomEntry
create_entry_generator<RandomEntry>(const std::string &args) {
  std::istringstream iss{args};
  uint64_t entry_bit_size;
  size_t seed;
  compressed_element_type min_value, max_value;
  iss >> entry_bit_size;
  if (!iss) {
    throw std::runtime_error("Arguments of RandomEntry generator creator"
                             "must contain \"entry_bit_size\"");
  }
  iss >> seed;
  if (!iss) {
    return RandomEntry(entry_bit_size);
  }
  iss >> min_value;
  if (!iss) {
    return RandomEntry(entry_bit_size, seed);
  }
  iss >> max_value;
  if (!iss) {
    return RandomEntry(entry_bit_size, seed, min_value);
  }
  return RandomEntry(entry_bit_size, seed, min_value, max_value);
}

template <>
inline PositionedEntry
create_entry_generator<PositionedEntry>(const std::string &args) {
  std::istringstream iss{args};
  uint64_t entry_bit_size;
  compressed_element_type min_value, max_value;
  int32_t step;
  iss >> entry_bit_size;
  if (!iss) {
    throw std::runtime_error("Arguments of PositionedEntry generator creator"
                             "must contain \"entry_bit_size\"");
  }
  iss >> step;
  if (!iss) {
    return PositionedEntry(entry_bit_size);
  }
  iss >> min_value;
  if (!iss) {
    return PositionedEntry(entry_bit_size, step);
  }
  iss >> max_value;
  if (!iss) {
    return PositionedEntry(entry_bit_size, step, min_value);
  }
  return PositionedEntry(entry_bit_size, step, min_value, max_value);
}

} // namespace tools::gen::imdb

#endif // TABLE_COLUMN_ENTRY_GENS_CREATOR_H
