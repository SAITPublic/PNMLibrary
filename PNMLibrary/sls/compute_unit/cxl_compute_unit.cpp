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

#include "sls/compute_unit/cxl_compute_unit.h"

#include "sls/compute_unit/compute_unit.h"

#include "core/device/sls/base.h"
#include "core/device/sls/constants.h"
#include "core/memory/barrier.h"

#include "common/compiler_internal.h"
#include "common/make_error.h"

#include "pnmlib/core/device.h"

#include "pnmlib/common/views.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <thread>
#include <type_traits>

namespace {
void poll_register(volatile std::atomic<uint32_t> *reg, uint32_t target) {
  using namespace std::chrono_literals;

  // Here we will read the data region with size equal to cache line size
  // This is a legacy from MD's PoC, but it makes device more stable
  static constexpr auto size =
      hardware_destructive_interference_size / sizeof(uint32_t);

  std::array<uint32_t, size> tmp;
  do {
    std::copy_n(reinterpret_cast<volatile uint32_t *>(reg), size, tmp.data());
    mfence();
    if (tmp[0] != target) {
      std::this_thread::yield();
    }
  } while (tmp[0] != target);
}
} // namespace

namespace pnm::sls {

CXLComputeUnit::CXLComputeUnit(device::BaseDevice *device, uint8_t pack)
    : device_(device) {
  while (!device_->acquire_cunit_for_write_from_range(&cunit_id_, pack)) {
    std::this_thread::yield();
  }

  enable();

  // [TODO: @y-lavrinenko] Get rid when simulator will be refactored
  if constexpr (PNM_PLATFORM == HARDWARE) {
    poll(PollType::CONTROL_SWITCH);
  }
}

void CXLComputeUnit::disable() {
  device_->control_switch_register(cunit_id_)->store(
      sls::device::BaseDevice::SLS_DISABLE_VALUE);
}

void CXLComputeUnit::enable() {
  device_->control_switch_register(cunit_id_)->store(
      sls::device::BaseDevice::SLS_ENABLE_VALUE);
}

CXLComputeUnit::~CXLComputeUnit() {
  disable();

  // [TODO: @y-lavrinenko] Get rid when simulator will be refactored
  if constexpr (PNM_PLATFORM == HARDWARE) {
    // Wait for control-switch. This is not mentioned in documentation but
    // allows to process sequential requests without hang
    poll(PollType::ENABLE);
  }

  device_->release_cunit_for_write(cunit_id_);
}

void CXLComputeUnit::run() {
  run_async();
  poll(PollType::FINISH);
}

void CXLComputeUnit::run_async() {
  device_->exec_sls_register(cunit_id_)->store(
      sls::device::BaseDevice::SLS_ENABLE_VALUE);
}

void CXLComputeUnit::wait() { poll(PollType::FINISH); }

void CXLComputeUnit::poll(PollType type) {
  switch (type) {
  case PollType::ENABLE:
    poll_register(device_->control_switch_register(cunit_id_),
                  sls::device::BaseDevice::SLS_ENABLE_VALUE);
    break;
  case PollType::FINISH:
    poll_register(device_->psum_poll_register(cunit_id_),
                  pnm::sls::device::POLL_FINISH_VALUE);
    break;
  case PollType::CONTROL_SWITCH:
    poll_register(device_->control_switch_register(cunit_id_),
                  sls::device::BaseDevice::SLS_CONTROL_SWITCHED_VALUE);
    break;
  default:
    throw pnm::error::make_not_sup(
        "Poll type: {},  device {}", std::underlying_type_t<PollType>(type),
        std::underlying_type_t<Device::Type>(device_->get_device_type()));
  }
}

void CXLComputeUnit::read_buffer(BlockType type,
                                 pnm::common_view<uint8_t> data) {
  Parent::read_buffer(device_, cunit_id_, type, data);
}

void CXLComputeUnit::write_buffer(BlockType type,
                                  pnm::common_view<const uint8_t> data) {
  Parent::write_buffer(device_, cunit_id_, type, data);
}

} // namespace pnm::sls
