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

#ifndef PNM_VARIABLE_ROW_VIEW_H
#define PNM_VARIABLE_ROW_VIEW_H

#include "views.h"

#include <cstddef>
#include <iterator>

namespace pnm {
/*! \brief Rowwise view with variable row size
 *
 * This class represents rowwise view with variable row size specified by
 * row_size view. The number of rows is equal to row_size.size(), each row
 * has size equal to the respective value from row_size view. There is no
 * leading and trailing gap in the row.
 *
 * @tparam T -- type of elements in view
 * @tparam I -- type of elements in row_size view with row's lengths
 * */
template <typename T, typename I = const unsigned> class variable_row_view {
public:
  variable_row_view() = default;
  variable_row_view(T *begin, T *end, common_view<I> row_size)
      : begin_{begin}, end_{end}, row_size_{row_size} {}

  template <typename R> struct views_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = common_view<R>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    views_iterator(R *base, const I *row_size,
                   const variable_row_view<T, I> *view)
        : base_{base}, row_size_{row_size}, view_{view} {
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
      base_ += *row_size_;
      ++row_size_;
      update();
      return *this;
    }

    auto operator++(int) {
      auto clone = *this;
      ++(*this);
      return clone;
    }

    auto &operator--() {
      --row_size_;
      base_ -= *row_size_;
      update();
      return *this;
    }

    auto operator--(int) {
      auto clone = *this;
      --(*this);
      return clone;
    }

  private:
    void update() {
      if (row_size_ != view_->row_size_.end()) {
        current_ = make_view(base_, base_ + *row_size_);
      } else {
        current_ = make_view(base_, base_);
      }
    }
    R *base_;
    const I *row_size_;
    value_type current_;
    const variable_row_view<T, I> *view_;
  };

  auto begin() { return views_iterator<T>(begin_, row_size_.begin(), this); }
  auto end() { return views_iterator<T>{end_, row_size_.end(), this}; }

  auto begin() const {
    return views_iterator<const T>(const_cast<const T *>(begin_),
                                   row_size_.begin(), this);
  }
  auto end() const {
    return views_iterator<const T>{const_cast<const T *>(end_), row_size_.end(),
                                   this};
  }

  auto size() const { return row_size_.size(); }

private:
  T *begin_ = nullptr, *end_ = nullptr;
  common_view<I> row_size_ = {};
};

template <typename T, typename I = const unsigned>
auto make_variable_row_view(T *begin, T *end, common_view<I> row_size) {
  return variable_row_view(begin, end, row_size);
}
} // namespace pnm

#endif // PNM_VARIABLE_ROW_VIEW_H
