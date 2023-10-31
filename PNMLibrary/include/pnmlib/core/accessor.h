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

#ifndef _PNM_ACCESSOR_H_
#define _PNM_ACCESSOR_H_

#include "memory.h"

#include <cassert>
#include <cstdint>
#include <iterator>
#include <memory>
#include <utility>

namespace pnm::memory {

/** @brief Base class that define device-specific access logic. */
class AccessorCore {
public:
  template <typename T> const T &access(uint64_t index) const {
    return *reinterpret_cast<const T *>(
        access_impl(index * sizeof(T), sizeof(T)));
  }

  template <typename T> void store(T value, uint64_t index) {
    store_impl(&value, sizeof(T), index * sizeof(T));
  }

  auto virtual_range() const { return virtual_range_impl(); }
  auto phys_range() const { return phys_range_impl(); }

  auto bytes() const { return bytes_impl(); }

  virtual ~AccessorCore() = default;

private:
  /** @brief Interface to get pointer for the `byte_offset` data byte */
  virtual const uint8_t *access_impl(uint64_t byte_offset,
                                     uint64_t size) const = 0;

  virtual VirtualRegion virtual_range_impl() const = 0;
  virtual DeviceRegion phys_range_impl() const = 0;

  virtual uint64_t bytes_impl() const = 0;

  virtual void store_impl(const void *value, uint64_t size,
                          uint64_t byte_offset) = 0;
};

template <typename T> class AccessorProxyRef;
template <typename T> class Buffer;

/** @brief This is a facade class that provides user-friendly access to the
 * "device" memory */
template <typename T> class Accessor {
public:
  class Iterator;

  template <typename Core, typename... Args>
  static Accessor create(Args &&...args) {
    return Accessor{std::make_unique<Core>(std::forward<Args>(args)...)};
  }

  AccessorProxyRef<T> at(uint64_t index) {
    return AccessorProxyRef(*this, index);
  }

  const T &at(uint64_t index) const { return base_->access<T>(index); }

  AccessorProxyRef<T> operator[](uint64_t index) { return at(index); }

  const T &operator[](uint64_t index) const { return at(index); }

  auto begin() { return Iterator(*this, 0); }

  auto end() { return Iterator(*this, size()); }

  auto size() const { return base_->bytes() / sizeof(T); }

  auto virtual_range() const { return base_->virtual_range(); }
  auto phys_range() const { return base_->phys_range(); }

  void store(const T &value, uint64_t index) { base_->store(value, index); }

private:
  explicit Accessor(std::unique_ptr<AccessorCore> base)
      : base_{std::move(base)} {
    assert(base_->bytes() % sizeof(T) == 0 &&
           "Fail to create accessor for memory region. Region should "
           "contain integral number of elements T.");
  }

  template <typename U> friend class pnm::memory::Buffer;

  std::unique_ptr<AccessorCore> base_;
};

/** @brief Proxy object to handle complex store process for device memory */
template <typename T> class AccessorProxyRef {
public:
  AccessorProxyRef(Accessor<T> &base, uint64_t index)
      : base_{&base}, index_{index} {}

  AccessorProxyRef &operator=(T value) {
    base_->store(value, index_);
    return *this;
  }

  operator T() { return const_cast<const Accessor<T> *>(base_)->at(index_); }
  operator T() const {
    return const_cast<const Accessor<T> *>(base_)->at(index_);
  }

private:
  Accessor<T> *base_;
  uint64_t index_;
};

template <typename T> class Accessor<T>::Iterator {
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = AccessorProxyRef<T>;
  using difference_type = uint64_t;
  using reference = AccessorProxyRef<T>;
  using pointer = AccessorProxyRef<T>;

  Iterator(Accessor<T> &base, uint64_t position)
      : base_{&base}, current_index_{position} {}

  reference operator*() { return base_->at(current_index_); }
  reference operator->() { return base_->at(current_index_); }
  reference operator[](uint64_t index) {
    return base_->at(current_index_ + index);
  }

  auto &operator++() {
    ++current_index_;
    return *this;
  }

  auto operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
  }

  auto &operator--() {
    --current_index_;
    return *this;
  }

  auto operator--(int) {
    auto tmp = *this;
    --(*this);
    return tmp;
  }

  auto &operator+=(uint64_t n) {
    current_index_ += n;
    return *this;
  }

  auto &operator-=(uint64_t n) {
    current_index_ -= n;
    return *this;
  }

  auto operator+(difference_type lhs) const {
    auto copy = *this;
    return copy += lhs;
  }

  auto operator-(difference_type lhs) const {
    auto copy = *this;
    return copy -= lhs;
  }

  difference_type operator-(const Accessor::Iterator &rhs) const {
    return current_index_ - rhs.current_index_;
  }

  bool operator!=(const Accessor::Iterator &rhs) const {
    return (base_ == rhs.base_) && current_index_ != rhs.current_index_;
  }

  bool operator==(const Accessor::Iterator &rhs) const {
    return !(*this != rhs);
  }

private:
  Accessor *base_;
  uint64_t current_index_;
};
} // namespace pnm::memory
#endif //_PNM_ACCESSOR_H_
