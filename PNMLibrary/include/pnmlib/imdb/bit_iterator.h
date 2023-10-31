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

#ifndef _IMDB_BIT_ITERATOR_H_
#define _IMDB_BIT_ITERATOR_H_

#include <cstdint>
#include <iterator>

namespace pnm::views {
template <typename T, typename C> struct bit_iterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = T;
  using difference_type = uint64_t;
  using pointer = T;
  using reference = T;

  constexpr bit_iterator(const C &base, uint64_t index)
      : base_{base}, index_{index} {}

  constexpr bool operator!=(const bit_iterator &rhs) const {
    return index_ != rhs.index_;
  }

  constexpr bool operator==(const bit_iterator &rhs) const {
    return index_ == rhs.index_;
  }

  constexpr auto &operator++() {
    index_ = base_.it_helper_next(index_);
    return *this;
  }

  constexpr auto operator++(int) {
    auto copy = *this;
    ++(*this);
    return copy;
  }

  constexpr reference operator*() const {
    return base_.it_helper_value(index_);
  }
  constexpr reference operator*() { return base_.it_helper_value(index_); }

private:
  const C &base_;
  uint64_t index_;
};

} // namespace pnm::views

#endif //_IMDB_BIT_ITERATOR_H_
