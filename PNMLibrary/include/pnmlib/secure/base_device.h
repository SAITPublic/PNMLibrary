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

#ifndef _SLS_SECURE_BASE_DEVICE_H_
#define _SLS_SECURE_BASE_DEVICE_H_

#include "pnmlib/common/views.h"

#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>

namespace pnm::sls::secure {

// [TODO: @a-korzun] use `std::any` instead of this class, since SGX
// support is no longer a concern.
/*! \brief Typeless device arguments for init method.
 *
 * We need this class to forward arguments for device from user code to device
 * class. Different devices have different set of arguments, so we should pack
 * it into single typeless object to make forwarding process easier.
 *
 * This class is exported in public headers to PyTorch, which uses C++14,
 * so we cannot use `std::any` in the implementation.  Instead, we use type
 * erasure based on external polymorphism here.
 *
 * */
class DeviceArguments {
public:
  template <typename T>
  explicit DeviceArguments(T value)
      : device_args_{std::make_unique<ValueImpl<T>>(std::move(value))} {}

  template <typename T> const T &get() const {
    const ValueInterface &interface_ref = *device_args_;
    const auto &impl_ref = dynamic_cast<const ValueImpl<T> &>(interface_ref);
    return impl_ref.get();
  }

  template <typename T> T &get() {
    // Call `get() const` and cast the result to a non-const reference.
    // This is safe to do because this method can only be called on non-const
    // `*this`. Do this to avoid code duplication with `get() const`.
    const auto &const_self = *this;
    return const_cast<T &>(const_self.get<T>());
  }

private:
  // Interface that any object that `DeviceArguments` can store should satisfy.
  class ValueInterface {
  public:
    virtual ~ValueInterface() = default;
  };

  // Implementation of this interface for all types.
  template <typename T> class RawValueImpl : public ValueInterface {
  public:
    // Additional constructor.
    template <typename... Args>
    RawValueImpl(Args &&...args) : value_(std::forward<Args>(args)...) {}

    // Additional getters.
    T &get() { return value_; }
    const T &get() const { return value_; }

  private:
    T value_;
  };

  // Remove cv-qualification from T so that `const T` and `T` are considered the
  // same type.
  template <typename T> using ValueImpl = RawValueImpl<std::remove_cv_t<T>>;

  std::unique_ptr<ValueInterface> device_args_;
};

/*! \brief Base class for NDP device */
class IDevice {
public:
  void init(const DeviceArguments *args) { init_impl(args); }
  void load(const void *src, uint64_t size_bytes) {
    load_impl(src, size_bytes);
  }

  void operator()(uint64_t minibatch_size,
                  pnm::views::common<const uint32_t> lengths,
                  pnm::views::common<const uint32_t> indices,
                  pnm::views::common<uint8_t> psum) {
    run_impl(minibatch_size, lengths, indices, psum);
  }

  virtual ~IDevice() = default;

private:
  virtual void init_impl(const DeviceArguments *args) = 0;
  virtual void load_impl(const void *src, uint64_t size_bytes) = 0;

  virtual void run_impl(uint64_t minibatch_size,
                        pnm::views::common<const uint32_t> lengths,
                        pnm::views::common<const uint32_t> indices,
                        pnm::views::common<uint8_t> psum) = 0;
};

} // namespace pnm::sls::secure

#endif //_SLS_SECURE_BASE_DEVICE_H_
