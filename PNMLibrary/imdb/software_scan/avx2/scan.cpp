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

#include "scan.h"

#include "imdb/software_scan/avx2/load.h"
#include "imdb/software_scan/avx2/predicate.h"
#include "imdb/software_scan/avx2/store.h"

#include "common/compiler_internal.h"

#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/common/views.h"

#include <immintrin.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace pnm::imdb::internal::avx2 {
static constexpr auto CONSTANTS_COUNT = 19UL;

const std::array<DecompressionConstants, CONSTANTS_COUNT> &ConstantsSet() {
  static const std::array<DecompressionConstants, CONSTANTS_COUNT> constants =
      []() {
        std::array<DecompressionConstants, CONSTANTS_COUNT> cts{};
        for (auto i = 1UL; i < cts.size(); ++i) {
          cts[i] = DecompressionConstants(i);
        }
        return cts;
      }();
  return constants;
}

/** @brief Implement scan operation in `column` with predicate `predicate`
 * and store result to `result` container
 */
template <typename Container, typename Predicate, typename Result>
size_t scan_avx2_impl(Container column, Predicate predicate, Result &result,
                      DecompressionConstants constant) {
  static constexpr auto block_size = 8;

  auto count = 0UL;

  auto i = 0UL;
  const auto *base = reinterpret_cast<const uint8_t *>(column.data());
  for (; i < column.size(); i += block_size) {
    // load and decompress column's values
    const auto data = load_with_decompression(base, constant);

    // apply predicate to 8 values
    const auto mask = apply_predicate(data, predicate);

    // call store function and invert bits in mask to select in-range values
    count += store(i, mask, result, count);

    base += column.value_bits();
  }

  return count;
}

namespace {
template <typename Output>
ALWAYS_INLINE size_t in_range_list_impl(const compressed_vector_view &column,
                                        predictor_input_vector_view predicate,
                                        Output result) {
  const auto max_value = _mm256_set1_epi32(((predicate.size() - 1) >> 5) + 1);
  return scan_avx2_impl(column, AVXInList{max_value, predicate}, result,
                        ConstantsSet()[column.value_bits()]);
}

template <typename Output>
ALWAYS_INLINE size_t in_range_scan_impl(const compressed_vector_view &column,
                                        RangeOperation range, Output result) {
  const auto min = _mm256_set1_epi32(range.start);
  const auto max = _mm256_set1_epi32(range.end);
  return scan_avx2_impl(column, AVXInRange{min, max}, result,
                        ConstantsSet()[column.value_bits()]);
}
} // namespace

size_t scan(compressed_vector_view column, RangeOperation range,
            index_vector_view result) {
  return in_range_scan_impl(column, range, result);
}

size_t scan(compressed_vector_view column, RangeOperation range,
            bit_vector_view result) {
  return in_range_scan_impl(column, range, result);
}

size_t scan(compressed_vector_view column,
            predictor_input_vector_view predicate, index_vector_view result) {
  return in_range_list_impl(column, predicate, result);
}

size_t scan(compressed_vector_view column,
            predictor_input_vector_view predicate, bit_vector_view result) {
  return in_range_list_impl(column, predicate, result);
}
} // namespace pnm::imdb::internal::avx2
