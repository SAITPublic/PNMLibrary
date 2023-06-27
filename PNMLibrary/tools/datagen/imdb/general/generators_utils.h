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

#ifndef IMDB_GENERATORS_UTILS_H
#define IMDB_GENERATORS_UTILS_H

#include "pnmlib/imdb/scan_types.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace tools::gen::imdb {

using namespace pnm::imdb;

inline auto value_per_request_count(uint32_t column_range_size,
                                    uint32_t column_size, double selectivity) {
  assert(selectivity >= 0. && selectivity <= 1.);
  const double average_value_duplicates =
      column_size / static_cast<double>(column_range_size);
  const double target_result_size = column_size * selectivity;
  const double request_range_size =
      std::floor(target_result_size / average_value_duplicates);
  return static_cast<uint32_t>(request_range_size);
}

inline size_t
calc_entries_count(const compressed_vector &column,
                   const predictor_input_vector &predicate_vector) {
  return std::count_if(column.begin(), column.end(), [&](const auto value) {
    return predicate_vector.contains(value);
  });
}

inline size_t calc_entries_count(const compressed_vector &column,
                                 const RangeOperation &range) {

  return std::count_if(column.begin(), column.end(), [&](const auto value) {
    return value >= range.start && value <= range.end;
  });
}

inline double
calc_actual_selectivity(const compressed_vector &column,
                        const predictor_input_vector &predicate_vector) {

  const auto count = calc_entries_count(column, predicate_vector);
  return count / static_cast<double>(column.size());
}

inline double calc_actual_selectivity(const compressed_vector &column,
                                      const RangeOperation &range) {
  const auto count = calc_entries_count(column, range);
  return count / static_cast<double>(column.size());
}

} // namespace tools::gen::imdb

#endif // IMDB_GENERATORS_UTILS_H
