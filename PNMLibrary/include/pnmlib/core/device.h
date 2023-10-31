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

#ifndef _PNM_DEVICE_H_
#define _PNM_DEVICE_H_

#include "pnmlib/common/compiler.h"
#include "pnmlib/common/definitions.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace pnm {
class PNM_API Device {
public:
  enum class Type : uint8_t { SLS, IMDB };
  enum class ResetOptions : uint8_t {
    SoftwareOnly = 0,
    HardwareOnly = 1,
    SoftwareHardware = 2,
  };
  Device() = default;
  virtual ~Device() = default;
  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;
  // [TODO: @e-kutovoi] Enable move of device
  Device(Device &&) = delete;
  Device &operator=(Device &&) = delete;

  // [TODO: @e-kutovoi] Make this a free function after putting everything into
  // a namespace
  static std::unique_ptr<Device> make_device(Type type);

  template <typename T> T *as() {
    if constexpr (DEBUG) {
      T *to_device = dynamic_cast<T *>(this);
      assert(to_device);
      return to_device;
    } else {
      T *to_device = static_cast<T *>(this);
      return to_device;
    }
  }

  template <typename T> const T *as() const {
    if constexpr (DEBUG) {
      const T *to_device = dynamic_cast<const T *>(this);
      assert(to_device);
      return to_device;
    } else {
      const T *to_device = static_cast<const T *>(this);
      return to_device;
    }
  }

  size_t memory_size() const { return memory_size_impl(); }

  bool get_resource_cleanup() const { return get_resource_cleanup_impl(); };

  void set_resource_cleanup(bool state) const {
    set_resource_cleanup_impl(state);
  }

  uint64_t get_leaked() const { return get_leaked_impl(); }

  void reset(ResetOptions options = ResetOptions::SoftwareOnly) {
    reset_impl(options);
  }
  virtual int get_devmem_fd() const = 0;
  virtual int get_resource_fd() const = 0;
  virtual Type get_device_type() const = 0;

protected:
  virtual size_t memory_size_impl() const = 0;
  virtual void reset_impl(ResetOptions options) = 0;
  virtual bool get_resource_cleanup_impl() const = 0;
  virtual void set_resource_cleanup_impl(bool state) const = 0;
  virtual uint64_t get_leaked_impl() const = 0;
};

using DevicePointer = std::unique_ptr<Device>;

} // namespace pnm

#endif /* _PNM_DEVICE_H_ */
