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

#ifndef PNM_ROWWISE_VIEW_H
#define PNM_ROWWISE_VIEW_H

#include "views.h"

#include <cstddef>
#include <iterator>

namespace pnm::views {
/*! \brief Class presents contiguous memory range as an array of views
 *
 * Class provides access to single constant contiguous memory block like it
 * contains multiple rows with fixed size.
 *
 * [e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10, e11, e12, e13, e14, e15]
 *  [----|---------------|-------][----|---------------|-------][-------
 *  | logical_start  logical_end || logical_start  logical_end ||-------
 *  |--->|front_gap      |       ||--->|front_offset   |       ||-------
 *  |    |       back_gap|<------||    |       back_gap|<------||-------
 *  |<---------row_size--------->||<---------row_size--------->||-------
 *  ^                            ^^                            ^^-------
 *  |                            ||                            ||-------
 *  physical_start    physical_end|                            ||-------
 *
 *  A row is defined by:
 *   - row_size -- one physical row size in bytes
 *   - front_gap -- a gap between physical and logical row starts
 *   - back_gap -- a gap between logical and physical row ends
 * @tparam T type of elements in array
 * */
template <typename T> class rowwise {
public:
  rowwise() = default;
  rowwise(T *begin, T *end, size_t row_size, size_t front_gap, size_t back_gap)
      : begin_{begin}, end_{end}, row_physical_size_{row_size},
        row_logical_size_{row_size - back_gap - front_gap},
        front_gap_{front_gap},
        size_(std::distance(begin_, end_) / row_physical_size_) {}

  rowwise(T *begin, T *end, size_t row_size)
      : rowwise(begin, end, row_size, 0, 0) {}

  template <typename R> struct views_iterator {
    using iterator_category = std::random_access_iterator_tag;
    using value_type = common<R>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    views_iterator(R *base, size_t step, size_t offset, size_t size)
        : base_{base}, step_{step}, offset_{offset}, size_{size} {
      update();
    }

    reference operator*() { return current_; }
    reference operator*() const { return current_; }

    pointer operator->() { return &current_; }
    pointer operator->() const { return &current_; }

    bool operator==(const views_iterator &rhs) const {
      return base_ == rhs.base_;
    }
    bool operator!=(const views_iterator &rhs) const { return !(*this == rhs); }

    auto &operator++() {
      base_ += step_;
      update();
      return *this;
    }

    auto operator++(int) {
      auto clone = *this;
      ++(*this);
      return clone;
    }

    auto &operator+=(difference_type n) {
      base_ += step_ * n;
      update();
      return *this;
    }

    auto &operator-=(difference_type n) { return (*this) += -n; }

    friend auto operator+(views_iterator lhs, difference_type n) {
      return lhs += n;
    }
    friend auto operator+(difference_type n, views_iterator rhs) {
      return rhs += n;
    }

    friend auto operator-(views_iterator lhs, difference_type n) {
      return lhs -= n;
    }
    friend auto operator-(difference_type n, views_iterator rhs) {
      return rhs -= n;
    }

  private:
    void update() { current_ = make_view(base_ + offset_, size_); }
    R *base_;
    size_t step_, offset_, size_;
    value_type current_;
  };

  auto begin() {
    return views_iterator<T>(begin_, row_physical_size_, front_gap_,
                             row_logical_size_);
  }
  auto end() { return views_iterator<T>{end_, 0, 0, 0}; }

  auto begin() const {
    return views_iterator<const T>(const_cast<const T *>(begin_),
                                   row_physical_size_, front_gap_,
                                   row_logical_size_);
  }
  auto end() const {
    return views_iterator<const T>{const_cast<const T *>(end_), 0, 0, 0};
  }

  auto size() const { return size_; }

private:
  T *const begin_ = nullptr, *const end_ = nullptr;
  size_t row_physical_size_{0}, row_logical_size_{0};
  size_t front_gap_{0};
  const size_t size_ = {0};
};

template <typename T>
auto make_rowwise_view(T *begin, T *end, size_t row_size, size_t front_gap,
                       size_t back_gap) {
  return rowwise(begin, end, row_size, front_gap, back_gap);
}

template <typename T>
auto make_rowwise_view(T *begin, T *end, size_t row_size) {
  return make_rowwise_view(begin, end, row_size, 0, 0);
}

} // namespace pnm::views

#endif // PNM_ROWWISE_VIEW_H
