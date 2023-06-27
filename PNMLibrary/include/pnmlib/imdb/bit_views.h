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

#ifndef _IMDB_BIT_VIEWS_H_
#define _IMDB_BIT_VIEWS_H_

#include "pnmlib/imdb/bit_common.h"
#include "pnmlib/imdb/bit_iterator.h"

#include "pnmlib/common/misc_utils.h"
#include "pnmlib/common/views.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <type_traits>

namespace pnm::views {

/** @brief View to provide access for compressed data in a region
 *
 * This class provides methods to read and write values in bit reduced format
 * (https://confluence.samsungds.net/display/NP/IMDB+PoC+Pipeline) into external
 * preallocated memory region. The value bit size is defined by `value_bits` in
 * ctors. The values count is defined by `n` in ctor.
 *
 * The size of underlying memory should be aligned by sizeof(T) * 2 where T is
 * memory region type. The mapping of nonaligned region leads to UB due to
 * possible out-of-range access.
 *
 * @tparam T value type, should be unsigned integral type
 * @tparam C type of mapped memory region. Should be contiguous range with RA.
 * sls::common_view<T> is used by default.
 */
template <typename T, typename C = pnm::common_view<T>>
class bit_compressed_view {
  static_assert(std::is_unsigned_v<T>, "value type must be unsigned!");
  static_assert(std::is_same_v<T, typename C::value_type>,
                "value type and the type of container elements should match!");

public:
  constexpr bit_compressed_view() = default;
  constexpr explicit bit_compressed_view(uint64_t value_bits, uint64_t n,
                                         C other)
      : value_bits_{value_bits}, value_counts_{n},
        value_mask_((T{1} << value_bits_) - 1), data_{std::move(other)} {}
  template <typename It, typename End>
  constexpr bit_compressed_view(uint64_t value_bits, uint64_t n, It first,
                                End last)
      : bit_compressed_view(value_bits, n, C{first, last}) {}

  template <typename R>
  constexpr bit_compressed_view(uint64_t value_bits, uint64_t n, R &&range)
      : bit_compressed_view(value_bits, n, range.begin(), range.end()) {}

  constexpr auto size() const { return value_counts_; }
  constexpr auto capacity() const {
    return bytes() * pnm::bitutils::BIT_IN_BYTE / value_bits_;
  }
  constexpr auto bytes() const {
    return pnm::utils::byte_size_of_container(data_);
  }
  constexpr auto value_bits() const { return value_bits_; }

  constexpr auto &container() { return data_; }
  constexpr const auto &container() const { return data_; }

  constexpr const auto *data() const { return data_.data(); }

  /** @brief Read value from memory region
   *
   * @param i value index in range [0, size() - 1]
   * @return value at i position
   */
  [[nodiscard]] constexpr auto get_value(uint64_t i) const {
    auto [index, shift] = bitutils::in_storage_bit_position<T>(i * value_bits_);
    if (value_bits_ + shift < storage_bits) {
      return (data_[index] >> shift) & value_mask_;
    }

    const auto part1_size = storage_bits - shift;
    const T part2_mask = value_mask_ >> part1_size;

    return ((data_[index + 1] & part2_mask) << part1_size) |
           (data_[index] >> shift);
  }

  /** @brief Write value to memory region
   *
   * @param i value index in range [0, size() - 1]
   * @param value value to write
   */
  constexpr void set_value(uint64_t i, T value) {
    auto [index, shift] = bitutils::in_storage_bit_position<T>(i * value_bits_);
    value &= value_mask_;
    if (value_bits_ + shift < storage_bits) {
      data_[index] |= value << shift;
    } else {
      const auto part1_size = storage_bits - shift;
      data_[index] |= (value & (value_mask_ >> (value_bits_ - part1_size)))
                      << shift;
      data_[index + 1] |= value >> part1_size;
    }
  }

  [[nodiscard]] constexpr auto begin() const { return iterator(*this, 0); }

  [[nodiscard]] constexpr auto end() const { return iterator(*this, size()); }

  [[nodiscard]] constexpr auto operator[](uint64_t i) const {
    return get_value(i);
  }

  /** @brief Class to provide common behaviour of non-constant operator[]
   */
  struct proxy_accessor {
    constexpr proxy_accessor &operator=(T value) && {
      storage_.set_value(index_, value);
      return *this;
    }
    constexpr operator T() && { return storage_.get_value(index_); }

  private:
    constexpr proxy_accessor(bit_compressed_view &st, uint64_t index)
        : storage_{st}, index_{index} {}
    friend bit_compressed_view;
    bit_compressed_view &storage_;
    uint64_t index_{};
  };

  [[nodiscard]] constexpr proxy_accessor operator[](uint64_t i) {
    return proxy_accessor(*this, i);
  }

protected:
  constexpr T it_helper_value(uint64_t index) const { return get_value(index); }
  constexpr uint64_t it_helper_next(uint64_t index) const { return ++index; }

  using iterator = bitview_iterator<T, bit_compressed_view<T, C>>;
  friend iterator;

  static constexpr auto storage_bits = sizeof(T) * 8;

  uint64_t value_bits_{1};
  uint64_t value_counts_{};
  std::decay_t<T> value_mask_{};
  C data_{};
};

template <typename It, typename End>
bit_compressed_view(uint64_t, uint64_t, It, End)
    -> bit_compressed_view<typename std::iterator_traits<It>::value_type>;

template <typename T>
bit_compressed_view(uint64_t, uint64_t, pnm::common_view<T>)
    -> bit_compressed_view<T>;

template <typename R>
bit_compressed_view(uint64_t, uint64_t, R &&) -> bit_compressed_view<
    typename std::iterator_traits<decltype(R::begin())>::value_type>;

/** @brief View to provide access for data in region stored as a BitVector
 *
 * This class provides methods to read/write data in external preallocated
 * memory region. The memory region is treated as a BitVector.
 * (https://confluence.samsungds.net/display/NP/IMDB+PoC+Pipeline)
 *
 * The region size should be aligned by sizeof(uint64_t). Using of nonaligned
 * memory leads to UB due to possible out-of-range access.
 *
 * @tparam C type of underlying memory region. Should be contiguous range. The
 * `sls::common_view` is used by default.
 */
template <typename C = pnm::common_view<uint64_t>> class bit_vector_view {
  static_assert(std::is_unsigned_v<typename C::value_type>,
                "value type must be unsigned!");

public:
  constexpr bit_vector_view() = default;
  constexpr explicit bit_vector_view(uint64_t max_value, C other)
      : max_value_{max_value}, data_{std::move(other)} {}

  template <typename It, typename End>
  constexpr bit_vector_view(uint64_t max_value, It first, End last)
      : bit_vector_view(max_value, C{first, last}) {}

  template <typename R>
  constexpr bit_vector_view(uint64_t max_value, R &&range)
      : bit_vector_view(max_value, range.begin(), range.end()) {}

  constexpr auto size() const { return max_value_; }
  constexpr auto capacity() const {
    return bytes() * pnm::bitutils::BIT_IN_BYTE;
  }
  constexpr auto bytes() const {
    return pnm::utils::byte_size_of_container(data_);
  }
  constexpr auto &container() { return data_; }
  constexpr const auto &container() const { return data_; }

  constexpr const auto *data() const { return data_.data(); }

  /** @brief Check is value in region or not
   *
   * @param value value in range [0, size() - 1]
   * @return true if value in memory, otherwise false
   */
  [[nodiscard]] constexpr bool contains(uint64_t value) const {
    if (value >= size()) {
      return false;
    }

    auto [index, mask] = bitutils::in_storage_bit_position_masked<T>(value);
    return data_[index] & mask;
  }

  /** @brief Insert value into BitVector.
   *
   * @param value value in range [0, size() - 1]
   */
  constexpr void insert(uint64_t value) {
    auto [index, mask] = bitutils::in_storage_bit_position_masked<T>(value);
    data_[index] |= mask;
  }

  /** @brief Erase value from container
   *
   * @param value value in range [0, size() - 1]
   */
  constexpr void erase(uint64_t value) {
    auto [index, mask] = bitutils::in_storage_bit_position_masked<T>(value);
    data_[index] &= ~mask;
  }

  constexpr void clear() { std::fill(data_.begin(), data_.end(), T{0}); }

  [[nodiscard]] constexpr auto begin() const {
    auto first_set = 0UL;
    while (first_set < size() && !contains(first_set)) {
      ++first_set;
    }

    return iterator(*this, first_set);
  }

  [[nodiscard]] constexpr auto end() const { return iterator(*this, size()); }

  [[nodiscard]] friend constexpr bool operator==(const bit_vector_view &lhs,
                                                 const bit_vector_view &rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
  }

protected:
  constexpr uint64_t it_helper_value(uint64_t index) const { return index; }
  constexpr uint64_t it_helper_next(uint64_t index) const {
    do {
      ++index;
    } while (index < size() && !contains(index));
    return index;
  }

  using iterator = bitview_iterator<uint64_t, bit_vector_view<C>>;
  friend iterator;

  using T = typename C::value_type;

  uint64_t max_value_{};
  C data_{};
};
} // namespace pnm::views

#endif //_IMDB_BIT_VIEWS_H_
