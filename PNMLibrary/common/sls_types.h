/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 */

#ifndef SLS_SLS_TYPES_H
#define SLS_SLS_TYPES_H

#include "common/compiler_internal.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <initializer_list>

namespace pnm::types {
template <typename T, unsigned N> class FixedVector {
public:
  using value_type = T;

  constexpr FixedVector() = default;

  constexpr explicit FixedVector(const T &value) { data_.fill(value); }

  constexpr FixedVector(std::initializer_list<T> const &ilist) {
    auto i = 0U;
    for (const auto &e : ilist) {
      data_[i++] = e;
    }
  }

  constexpr auto size() const { return data_.size(); }

  constexpr auto data() { return data_.data(); }
  constexpr auto data() const { return data_.data(); }

  constexpr auto begin() { return data_.begin(); }
  constexpr auto end() { return data_.end(); }

  constexpr auto begin() const { return data_.begin(); }
  constexpr auto end() const { return data_.end(); }

  constexpr T &operator[](size_t pos) { return data_[pos]; }
  constexpr const T &operator[](size_t pos) const { return data_[pos]; }

  FixedVector &operator+=(const FixedVector &rhs) {
    std::transform(begin(), end(), rhs.begin(), begin(), std::plus<T>{});
    return *this;
  }

  FixedVector &operator-=(const FixedVector &rhs) {
    auto minus = [](const auto &l, const auto &r)
                     NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW { return l - r; };

    std::transform(begin(), end(), rhs.begin(), begin(), minus);
    return *this;
  }

  constexpr bool operator==(const FixedVector &rhs) const {
    return data_ == rhs.data_;
  }

  constexpr bool operator!=(const FixedVector &rhs) const {
    return !(*this == rhs);
  }

private:
  std::array<T, N> data_;
};

template <typename T, unsigned N>
inline FixedVector<T, N> operator+(const FixedVector<T, N> &lhs,
                                   const FixedVector<T, N> &rhs) {
  FixedVector<T, N> output;
  std::transform(lhs.begin(), lhs.end(), rhs.begin(), output.begin(),
                 std::plus<T>{});
  return output;
}

template <typename T, unsigned N>
inline FixedVector<T, N> operator-(const FixedVector<T, N> &lhs,
                                   const FixedVector<T, N> &rhs) {
  FixedVector<T, N> output;
  std::transform(lhs.begin(), lhs.end(), rhs.begin(), output.begin(),
                 std::minus<T>{});
  return output;
}
} // namespace pnm::types

#endif // SLS_SLS_TYPES_H
