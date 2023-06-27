/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#ifndef _IMDB_BIT_COMMON_H_
#define _IMDB_BIT_COMMON_H_

#include <cassert>
#include <cstddef>
#include <utility>

namespace pnm::bitutils {

inline constexpr auto BIT_IN_BYTE = 8U;

/** @brief Returns the smallest aligned value >= size, where align is a power of
 * 2
 *
 * @param size
 * @param align
 * @return aligned size
 */
constexpr auto align_size(size_t size, size_t align) {
  assert(!(align & (align - 1)) && "Alignment should be power of 2");
  return (size + align - 1) & ~(align - 1);
}

/** @brief Return the size (number of elements of T) of storage that
 * can contains specified amount of bits with specified alignment
 *
 * @tparam T Underlying storage type
 * @param bits_count Number of bits to store
 * @param bit_align Size alignment in bits. Should be a power of 2 and greater
 * or equal to 8.
 * @return The number of sequential elements of type T
 */
template <typename T>
constexpr auto aligned_storage_size(size_t bits_count,
                                    size_t bit_align = 2 * sizeof(T) *
                                                       BIT_IN_BYTE) {
  assert(bit_align % 8 == 0 &&
         "bit_align should contain integral number of bytes");

  size_t bytes = (bits_count % BIT_IN_BYTE == 0)
                     ? bits_count / BIT_IN_BYTE
                     : (bits_count / BIT_IN_BYTE + 1);
  bytes = align_size(bytes, sizeof(T));

  return align_size(bytes, bit_align / BIT_IN_BYTE) / sizeof(T);
}

/** @brief Return the index and shift to get the index bit in sequential storage
 * of elements with type `storage_type`
 *
 * @tparam storage_type
 * @param index
 * @return pair cell index and bit index in cell
 */
template <typename storage_type>
constexpr auto in_storage_bit_position(size_t index) {
  const auto storage_bitsize = sizeof(storage_type) * BIT_IN_BYTE;
  const auto cell = index / storage_bitsize;
  const auto bit_shift = index & (storage_bitsize - 1);
  return std::pair{cell, bit_shift};
}

/** @brief Return the index and mask to get the index bit in sequential storage
 * of elements with type `storage_type`
 *
 * @tparam storage_type
 * @param index
 * @return pair cell index and mask for specified bit
 */
template <typename storage_type>
constexpr auto in_storage_bit_position_masked(size_t index) {
  const auto position = in_storage_bit_position<storage_type>(index);
  const auto bit_mask = 1UL << position.second;
  return std::pair{position.first, bit_mask};
}
} // namespace pnm::bitutils

#endif //_IMDB_BIT_COMMON_H_
