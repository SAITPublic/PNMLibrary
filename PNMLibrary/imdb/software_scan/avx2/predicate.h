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

#ifndef _AVX2_PREDICATE_H_
#define _AVX2_PREDICATE_H_

#include "common/compiler_internal.h"

#include "pnmlib/imdb/scan_types.h"

#include <immintrin.h>

#include <cstdint>

namespace pnm::imdb::internal::avx2 {
struct AVXInRange {
  __m256i min, max;
};

/** @brief Apply InRange predicate for each elements in `data` avx register
 *
 * @return uint8_t bitmask with 1 in i-th position if i-th data's element is in
 * range
 */
ALWAYS_INLINE uint8_t apply_predicate(__m256i data, const AVXInRange &range) {
  // The AVX2 doesn't support '<=' operation, so we perform not-in-range scan
  // here with further inversion of the result
  const auto ge = _mm256_cmpgt_epi32(range.min, data);
  const auto le = _mm256_cmpgt_epi32(data, range.max);
  auto out_range = _mm256_or_si256(ge, le);

  const uint8_t mask = _mm256_movemask_ps(_mm256_castsi256_ps(out_range));
  return mask ^ 0xFF;
}

struct AVXInList {
  __m256i max_index;
  pnm::imdb::predictor_input_vector_view list;
};

/** @brief Apply InList predicate for each elements in `data` avx register
 *
 * @return uint8_t bitmask with 1 in i-th position if i-th data's element is in
 * list
 */
ALWAYS_INLINE uint8_t apply_predicate(__m256i data,
                                      const AVXInList &predicate) {
  static const auto DIV32_SHIFT_V = _mm256_set1_epi32(5);
  static const auto MOD32_MASK_V = _mm256_set1_epi32(0b11111);
  static const auto ONE_V = _mm256_set1_epi32(1);

  // Value to load if mask is false.
  static const auto FAIL_V = _mm256_set1_epi32(0);

  const auto in_value_shift = _mm256_and_si256(data, MOD32_MASK_V);
  const auto value_indices = _mm256_srlv_epi32(data, DIV32_SHIFT_V);
  const auto detection_mask = _mm256_sllv_epi32(ONE_V, in_value_shift);

  const uint32_t *mem = predicate.list.data();

  static constexpr auto BYTE_SCALE = 4;

  const auto in_range = _mm256_cmpgt_epi32(predicate.max_index, value_indices);
  const auto in_list_containers =
      _mm256_mask_i32gather_epi32(FAIL_V, reinterpret_cast<const int *>(mem),
                                  value_indices, in_range, BYTE_SCALE);

  const auto in_list = _mm256_cmpeq_epi32(
      _mm256_and_si256(in_list_containers, detection_mask), detection_mask);

  return _mm256_movemask_ps(_mm256_castsi256_ps(in_list));
}
} // namespace pnm::imdb::internal::avx2

#endif // _AVX2_PREDICATE_H_
