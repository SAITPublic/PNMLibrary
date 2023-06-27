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

#ifndef _IMDB_ALIGNED_ALLOCATOR_H_
#define _IMDB_ALIGNED_ALLOCATOR_H_

#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

namespace pnm::utils {

/** @brief STL-friendly allocator for aligned memory allocation
 *
 * @tparam T
 * @tparam Alignment
 */
template <typename T, size_t Alignment> class aligned_allocator {
public:
  static_assert(Alignment != 0 && (Alignment & (Alignment - 1)) == 0,
                "The 'Alignment' is not a valid alignment value");

  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using is_always_equal = std::true_type;

  // The member template class rebind provides a way to obtain an allocator for
  // a different type. This struct is required by STL
  template <class U> struct rebind {
    using other = aligned_allocator<U, Alignment>;
  };

  aligned_allocator() = default;

  // CTOR to properly handle rebind allocator
  template <typename U>
  aligned_allocator(
      [[maybe_unused]] const aligned_allocator<U, Alignment> &other) {}

  pointer address(reference r) const { return &r; }
  const_pointer address(const_reference &s) const { return &s; }

  pointer allocate(size_type n) const {
    if (n == 0) {
      return nullptr;
    }
    return static_cast<pointer>(
        ::operator new[](n * sizeof(T), std::align_val_t(Alignment)));
  }

  void deallocate(pointer p, [[maybe_unused]] size_type n) const {
    ::operator delete[](p, std::align_val_t(Alignment));
  }

  size_t max_size() const {
    return std::numeric_limits<size_type>::max() / sizeof(T);
  }

  template <typename U, typename... Args>
  void construct(U *p, Args &&...args) const {
    new (static_cast<void *>(p)) U(std::forward<Args>(args)...);
  }
  template <typename U> void destroy(U *p) const { p->~U(); }

  // Required overloads to fit is_always_equal property
  bool operator!=([[maybe_unused]] const aligned_allocator &other) const {
    return false;
  }
  bool operator==([[maybe_unused]] const aligned_allocator &other) const {
    return true;
  }
};
} // namespace pnm::utils

#endif //_IMDB_ALIGNED_ALLOCATOR_H_
