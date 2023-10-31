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

#ifndef PNM_VIEWS_H
#define PNM_VIEWS_H

#include <sys/types.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>

namespace pnm::views {
/*! \brief Common view that refer to a constant contiguous sequence of data T.*/
template <typename T> class common {
public:
  using value_type = T;

  common() = default;
  common(T *begin, T *end) : begin_{begin}, end_{end} {}

  auto begin() { return begin_; }
  auto end() { return end_; }

  auto begin() const { return const_cast<const T *>(begin_); }
  auto end() const { return const_cast<const T *>(end_); }

  const T *data() const { return begin_; }
  T *data() { return begin_; }

  auto size() const { return std::distance(begin_, end_); }

  auto empty() const { return begin() == end(); }

  const T &operator[](size_t index) const { return *(begin_ + index); }

  T &operator[](size_t index) { return *(begin_ + index); }

private:
  T *begin_ = nullptr;
  T *end_ = nullptr;
};

template <typename T> inline auto make_view(T *first, T *last) {
  return common<T>{first, last};
}

template <typename T> inline auto make_view(T *first, size_t size) {
  return common<T>{first, first + size};
}

template <typename C> inline auto make_view(const C &c) {
  return common<const typename C::value_type>{c.data(), c.data() + c.size()};
}

template <typename C> inline auto make_view(C &c) {
  return common<typename C::value_type>{c.data(), c.data() + c.size()};
}

template <typename C> inline auto make_const_view(C &c) {
  return make_view(const_cast<const C &>(c));
}

template <typename D, typename S> inline auto view_cast(common<S> view) {
  return make_view(reinterpret_cast<D *>(view.begin()),
                   reinterpret_cast<D *>(view.end()));
}

template <typename D, typename S> inline auto view_const_cast(common<S> view) {
  return make_view(const_cast<D *>(view.begin()), const_cast<D *>(view.end()));
}

template <typename T> inline auto advance_view(common<T> view, off_t offset) {
  if (offset > 0) {
    assert(static_cast<uint64_t>(offset) < static_cast<uint64_t>(view.size()) &&
           "View overflow detected! Check offset parameter for "
           "validity!");
  }
  return make_view(view.begin() + offset, view.size() - offset);
}

/** @brief Compare the data pointed to by the views for equality.
 *
 * @returns true if sizes and contents of the viewed data regions are equal,
 * false otherwise.
 */
template <typename T>
inline bool operator==(const common<T> &lhs, const common<T> &rhs) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}
} // namespace pnm::views

#endif // PNM_VIEWS_H
