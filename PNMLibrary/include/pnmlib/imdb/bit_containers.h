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

#ifndef _IMDB_BIT_CONTAINERS_H_
#define _IMDB_BIT_CONTAINERS_H_

#include "pnmlib/imdb/aligned_allocator.h"
#include "pnmlib/imdb/bit_common.h"
#include "pnmlib/imdb/bit_views.h"

#include "pnmlib/common/views.h"

#include <cstdint>
#include <initializer_list>
#include <vector>

namespace pnm::containers {

// Align data by 32 bytes to apply AVX2 instruction for device offload
inline constexpr auto container_alignment = 32;

namespace details {

inline constexpr auto align_bits = 512;

template <typename T>
constexpr auto compressed_vector_storage_size(uint64_t value_bit_size,
                                              uint64_t N) {
  // extra bits to avoid out of range for scan and copy operations.
  constexpr auto EXTRA_BITS = 512;
  return bitutils::aligned_storage_size<T>(value_bit_size * N + EXTRA_BITS,
                                           align_bits);
}

template <typename T>
constexpr auto bit_vector_storage_size(uint64_t max_value) {
  return bitutils::aligned_storage_size<T>(max_value, align_bits);
}

} // namespace details

/** @brief A container that implements logic of compressed storage where values
 * are stored in reduced bit representation.
 *
 * Class represents a fixed size container that contains values with predefined
 * bit size. Class allocates memory that is required for proper access to store
 * N elements. The imdb specific alignment by 512 bits is applied to the
 * container size.
 *
 * See detail: https://confluence.samsungds.net/display/NP/IMDB+PoC+Pipeline
 *
 * Inherited methods from @see pnm::views::bit_compressed_view:
 *  -- size()
 *  -- capacity()
 *  -- bytes()
 *  -- data()
 *
 *  -- get_value(i)
 *  -- set_value(i)
 *  -- values()
 *
 *  -- operator[]
 *
 * @tparam T stored value type. Should be unsigned integral type.
 */
template <typename T = uint32_t,
          typename C = std::vector<
              T, pnm::utils::aligned_allocator<T, container_alignment>>>
class compressed_vector : public views::bit_compressed_view<T, C> {
public:
  constexpr compressed_vector() = default;

  /** @brief Create container with N elements with predefined bitsize.
   *
   * @param value_bit_size size of value in bit
   * @param N elements count
   */
  constexpr compressed_vector(uint64_t value_bit_size, uint64_t N)
      : base(value_bit_size, N, allocate(value_bit_size, N)) {}

  /** @brief Create container with std::range::distance(first, last) elements
   * and copy values from [first, last) range.
   *
   * @tparam It range start iterator
   * @tparam End range end sentinel
   * @param value_bits value size in bits
   * @param first the region start
   * @param last a past the last element in source region
   */
  template <typename It, typename End>
  constexpr compressed_vector(uint64_t value_bits, It first, End last)
      : compressed_vector(value_bits, first, last, std::distance(first, last)) {
  }

  /** @brief Container construction via initializer list
   *
   * @tparam U initializer list value's type
   * @param value_bits value size in bit
   * @param list source initializer list
   */
  template <typename U>
  constexpr compressed_vector(uint64_t value_bits,
                              std::initializer_list<U> list)
      : compressed_vector(value_bits, list.begin(), list.end()) {}

  constexpr auto view() const & {
    return views::bit_compressed_view<const T>(
        this->value_bits(), this->size(), this->container().data(),
        this->container().data() + this->container().size());
  }

  constexpr auto view() & {
    return views::bit_compressed_view<T>(
        this->value_bits(), this->size(), this->container().data(),
        this->container().data() + this->container().size());
  }

  constexpr auto view() const && = delete;

private:
  constexpr auto allocate(uint64_t value_bit_size, uint64_t N) {
    return inner_storage(
        details::compressed_vector_storage_size<T>(value_bit_size, N));
  }

  template <typename It, typename End>
  constexpr compressed_vector(uint64_t value_bits, It first, End last,
                              uint64_t size)
      : base(value_bits, size, allocate(value_bits, size)) {
    auto i = 0UL;
    for (auto vit = first; vit != last; ++vit) {
      this->set_value(i, *vit);
      ++i;
    }
  }

  using inner_storage = C;
  using base = views::bit_compressed_view<T, C>;
};

/** @brief Interpret an address as a compressed vector and make a
 * pnm::views::bit_compressed_view to it
 *
 * @param value_bits value size in bits
 * @param start address where data is stored
 * @param value_counts amount of elements in the vector
 *
 * @tparam T stored value type. Should be unsigned integral type.
 */
template <typename T>
constexpr auto compressed_view_from_raw(uint64_t value_bits, T *start,
                                        uint64_t value_counts) {
  T *end = start +
           details::compressed_vector_storage_size<T>(value_bits, value_counts);
  return views::bit_compressed_view<T>(value_bits, value_counts, start, end);
}

/** @brief A container that implements BitVector logic to store fixed amount of
 * values
 *
 * Class represent fixed size container where values stored as by setting a bit
 * in proper position. A class allocates memory to provide proper access to the
 * data in region. The imdb specific align by 512 bits is applied to the
 * container size.
 *
 * See detail: https://confluence.samsungds.net/display/NP/IMDB+PoC+Pipeline
 *
 * Inherited methods from @see pnm::views::bit_vector_view:
 *  -- size()
 *  -- capacity()
 *  -- bytes()
 *  -- data()
 *
 *  -- contains(value)
 *  -- insert(value)
 *  -- erase(value)
 *
 *  -- values()
 *
 *  -- clear()
 *
 * @tparam T
 */
template <typename T = uint64_t,
          typename C = std::vector<
              T, pnm::utils::aligned_allocator<T, container_alignment>>>
class bit_vector : public views::bit_vector_view<C> {
public:
  constexpr bit_vector() = default;

  /** @brief Create container to store values in range [0, N - 1]
   *
   * @param N the past the max value that can be stored in container
   */
  constexpr bit_vector(uint64_t N) : base(N, allocate(N)) {}

  /** @brief Create container to store value in range [0, max(first, last)] and
   * copy value from region
   *
   * @tparam It range start iterator
   * @tparam End range end sentinel
   * @param first the region start
   * @param last iterator of past the last element in region or sentinel
   */
  template <typename It, typename End>
  constexpr bit_vector(It first, End last)
      : bit_vector(first, last, range_upper_limit(first, last)) {}

  /** @brief Container construction via initializer list
   *
   * @tparam U initializer list value's type
   * @param list source initializer list
   */
  template <typename U>
  constexpr bit_vector(std::initializer_list<U> list)
      : bit_vector(list.begin(), list.end()) {}

  constexpr auto view() const & {
    return views::bit_vector_view<pnm::common_view<const T>>(
        this->size(), this->container().data(),
        this->container().data() + this->container().size());
  }

  constexpr auto view() & {
    return views::bit_vector_view<pnm::common_view<T>>(
        this->size(), this->container().data(),
        this->container().data() + this->container().size());
  }

  constexpr auto view() const && = delete;

private:
  template <typename It, typename End>
  constexpr auto range_upper_limit(It first, End last) {
    return (first == last) ? 0 : (*std::max_element(first, last) + 1);
  }

  constexpr auto allocate(uint64_t N) {
    return inner_storage(details::bit_vector_storage_size<T>(N));
  }

  // Delegating ctor to avoid multiple upper_limit evaluation
  template <typename It, typename End>
  constexpr bit_vector(It first, End last, uint64_t upper_limit)
      : base(upper_limit, allocate(upper_limit)) {
    for (auto vit = first; vit != last; ++vit) {
      this->insert(*vit);
    }
  }

  using inner_storage = C;
  using base = views::bit_vector_view<C>;
};

/** @brief Interpret an address as a bit vector and make a
 * pnm::views::bit_vector_view to it
 *
 * @param start address where data is located
 * @param max_value max element that can be contained in the vector
 *
 * @tparam T stored value type. Should be unsigned integral type.
 */
template <typename T>
static constexpr auto bit_view_from_raw(T *start, uint64_t max_value) {
  T *end = start + details::bit_vector_storage_size<T>(max_value);
  return views::bit_vector_view<pnm::common_view<T>>(max_value, start, end);
}

} // namespace pnm::containers

#endif //_IMDB_BIT_CONTAINERS_H_
