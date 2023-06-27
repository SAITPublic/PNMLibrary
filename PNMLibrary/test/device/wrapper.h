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

#ifndef WRAPPER_H
#define WRAPPER_H

#include "core/device/imdb/base.h"
#include "core/device/sls/base.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <variant>

//[TODO:] common interface for process manager functionality
class DeviceWrapper {
  using IMDBDevice = pnm::imdb::device::BaseDevice;
  using SLSDevice = pnm::sls::device::BaseDevice;
  using VariantType = std::variant<IMDBDevice *, SLSDevice *>;

public:
  DeviceWrapper(pnm::Device::Type type)
      : ctx_(pnm::make_context(type)), device_(get_device(ctx_)) {}

  void set_resource_cleanup(bool value) {
    ctx_->device()->set_resource_cleanup(value);
  }

  uint64_t get_resource_cleanup() {
    return ctx_->device()->get_resource_cleanup();
  }

  uint64_t get_leaked() { return ctx_->device()->get_leaked(); }

  /**@brief member function for acquiring rank/thread
   *
   * @param preferred_unit preferred unit
   * @return acquired compute unit
   * @note acquired unit may be differ from preferred
   */
  uint8_t acquire_compute_unit(uint8_t preferred_unit);

  void release_compute_unit(uint8_t compute_unit);

  uint64_t compute_unit_count();

  bool is_compute_unit_busy(uint8_t unit);

  uint64_t memory_size();

  pnm::ContextHandler &context() { return ctx_; }

private:
  static VariantType get_device(pnm::ContextHandler &ctx);

  pnm::ContextHandler ctx_;
  std::variant<IMDBDevice *, SLSDevice *> device_;
};

inline bool exchange_cleanup(bool new_value, pnm::Device::Type type) {
  DeviceWrapper dev(type);
  bool old{};
  EXPECT_NO_THROW(old = dev.get_resource_cleanup());
  EXPECT_NO_THROW(dev.set_resource_cleanup(new_value));
  return old;
}

inline void restore_cleanup(bool was_enabled, pnm::Device::Type type) {
  DeviceWrapper dev(type);
  EXPECT_NO_THROW(dev.set_resource_cleanup(was_enabled));
}

#endif // WRAPPER_H
