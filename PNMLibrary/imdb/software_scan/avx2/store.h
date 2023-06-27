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
#ifndef _AVX2_STORE_H_
#define _AVX2_STORE_H_

#include "common/compiler_internal.h"

#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/common/views.h"

#include <emmintrin.h>
#include <immintrin.h>
#include <x86intrin.h>

#include <cstddef>
#include <cstdint>

namespace pnm::imdb::internal::avx2 {
/** @brief Store indices to the result index-vector according to the mask
 *
 * The store procedure is pretty simple:
 * 1. Move all indices that have enabled bit in mask
 *   1.1 Extend mask to uint64_t where each bit mapped to the byte multiply by
 * 0xFF (0b01101001 -> 0x00FFFF00FF0000FF)
 *   1.2 Select permutation indices for _mm256_permutevar8x32_ps function
 *   1.3 Make permutation
 * 2. Call _mm256_storeu_si256 for vectorized load of reordered indices to
 * memory
 *
 * @return The number of real stored values
 */
ALWAYS_INLINE size_t store(size_t base, uint8_t mask,
                           pnm::imdb::index_vector_view &result,
                           size_t out_index) {
  // Predefined vector with elements
  static const auto offsets = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);

  auto indices = _mm256_add_epi32(_mm256_set1_epi32(base), offsets);

  const auto extended_mask = _pdep_u64(mask, 0x0101010101010101) * 0xFF;

  // Predefined permutation indices
  static constexpr auto selection_mask = 0x0706050403020100;

  const auto permute_mask = _pext_u64(selection_mask, extended_mask);
  const auto permute_vec =
      _mm256_cvtepu8_epi32(_mm_cvtsi64_si128(permute_mask));

  // Two casts epi32 -> ps and ps -> epi32 to satisfy gcc
  indices = _mm256_castps_si256(
      _mm256_permutevar8x32_ps(_mm256_castsi256_ps(indices), permute_vec));
  _mm256_storeu_si256(reinterpret_cast<__m256i_u *>(&result[out_index]),
                      indices);
  return _popcnt32(mask);
}

/** @brief Store scan result into the bitvector
 *
 * Store the mask into container.
 *
 * @return 1
 */
ALWAYS_INLINE size_t store([[maybe_unused]] size_t base, uint8_t mask,
                           pnm::imdb::bit_vector_view &result,
                           size_t out_index) {
  reinterpret_cast<uint8_t *>(result.container().data())[out_index] = mask;
  return 1;
}
} // namespace pnm::imdb::internal::avx2

#endif //_AVX2_STORE_H_
