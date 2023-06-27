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

#ifndef _AVX2_LOAD_H_
#define _AVX2_LOAD_H_

#include "common/compiler_internal.h"
#include "common/make_error.h"

#include "pnmlib/imdb/bit_common.h"

#include <emmintrin.h>
#include <immintrin.h>
#include <tmmintrin.h>

#include <array>
#include <cassert>
#include <cstdint>

namespace pnm::imdb::internal::avx2 {

struct DecompressionConstants {
  DecompressionConstants() = default;
  explicit DecompressionConstants(uint32_t bits) : entry_size(bits) {
    assert(entry_size <= 18 &&
           "The entry size larger than 18 bit have not supported yet");

    value_mask = _mm256_set1_epi32((1UL << bits) - 1);
    init_shuffle_control_fields();
    init_align_shift_field();
    init_crop_masks();
  }

  uint32_t entry_size;
  __m128i shuffle_control_l;
  __m128i shuffle_control_h;

  __m256i align_shift;
  __m256i value_mask;

  __m128i crop_mask, inverse_crop_mask;

  static constexpr auto single_simd_lane_size = sizeof(__m128i);
  static constexpr auto avx_size = sizeof(__m256i);
  static constexpr auto dwords_count = avx_size / sizeof(uint32_t);

  void init_shuffle_control_fields() {
    auto value_shuffle_bytes = [this](uint8_t value_index) {
      const auto byte_min =
          value_index * entry_size / pnm::bitutils::BIT_IN_BYTE;
      const auto byte_max =
          (entry_size * (value_index + 1) - 1) / pnm::bitutils::BIT_IN_BYTE;

      static constexpr auto shuffle_submask_size = sizeof(uint32_t);
      std::array<uint8_t, shuffle_submask_size> shuffle_submask{0, 0, 0, 0};

      for (auto i = byte_min, k = 0U; i <= byte_max; ++i) {
        shuffle_submask[k++] = i % single_simd_lane_size;
      }
      return shuffle_submask;
    };

    std::array<uint8_t, avx_size> shuffle_mask;
    for (auto i = 0U, k = 0U; i < dwords_count; ++i) {
      for (auto &e : value_shuffle_bytes(i)) {
        shuffle_mask[k++] = e;
      }
    }

    shuffle_control_l =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(shuffle_mask.data()));
    shuffle_control_h = _mm_loadu_si128(reinterpret_cast<const __m128i *>(
        shuffle_mask.data() + single_simd_lane_size));
  }

  void init_align_shift_field() {
    std::array<uint32_t, dwords_count> right_shift_v{};
    for (auto i = 0U; i < dwords_count; ++i) {
      right_shift_v[i] = i * entry_size % pnm::bitutils::BIT_IN_BYTE;
    }
    align_shift = _mm256_loadu_si256(
        reinterpret_cast<const __m256i *>(right_shift_v.data()));
  }

  void init_crop_masks() {
    if (entry_size < 17) {
      crop_mask = _mm_set1_epi8(-1);
    } else if (entry_size <= 18) {
      crop_mask =
          _mm_set_epi32(entry_size == 17 ? 0x0000FFFF : 0x000000FF, -1, -1, -1);
    } else {
      throw pnm::error::make_inval("Entry size: {}.", entry_size);
    }

    inverse_crop_mask = _mm_andnot_si128(crop_mask, _mm_set1_epi8(-1));
  }
};

ALWAYS_INLINE __m256i singlelane_shuffle(__m256i value,
                                         __m128i shuffle_control_h,
                                         __m128i shuffle_control_l) {
  auto low_lane = _mm256_castsi256_si128(value);
  auto shuffled_l = _mm_shuffle_epi8(low_lane, shuffle_control_l);
  auto shuffled_h = _mm_shuffle_epi8(low_lane, shuffle_control_h);

  return _mm256_set_m128i(shuffled_h, shuffled_l);
};

ALWAYS_INLINE __m256i crosslane_shuffle(__m256i value,
                                        __m128i shuffle_control_h,
                                        __m128i shuffle_control_l,
                                        __m128i crop_mask,
                                        __m128i inverse_crop_mask) {
  auto low_lane = _mm256_castsi256_si128(value);
  auto shuffled_l = _mm_shuffle_epi8(low_lane, shuffle_control_l);
  auto shuffled_h = _mm_shuffle_epi8(low_lane, shuffle_control_h);

  auto hi_lane = _mm256_extracti128_si256(value, 0b1);
  auto shuffled_h_extra = _mm_shuffle_epi8(hi_lane, shuffle_control_h);

  shuffled_h_extra = _mm_and_si128(shuffled_h_extra, inverse_crop_mask);
  shuffled_h = _mm_and_si128(shuffled_h, crop_mask);

  shuffled_h = _mm_or_si128(shuffled_h, shuffled_h_extra);

  return _mm256_set_m128i(shuffled_h, shuffled_l);
};

ALWAYS_INLINE __m256i load_with_decompression(
    const uint8_t *base,
    const pnm::imdb::internal::avx2::DecompressionConstants &constant) {
  const auto data = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(base));

  auto decompressed =
      (LIKELY(constant.entry_size <= 16))
          ? singlelane_shuffle(data, constant.shuffle_control_h,
                               constant.shuffle_control_l)
          : crosslane_shuffle(data, constant.shuffle_control_h,
                              constant.shuffle_control_l, constant.crop_mask,
                              constant.inverse_crop_mask);

  decompressed = _mm256_srlv_epi32(decompressed, constant.align_shift);

  decompressed = _mm256_and_si256(decompressed, constant.value_mask);

  return decompressed;
}

} // namespace pnm::imdb::internal::avx2

#endif //_AVX2_LOAD_H_
