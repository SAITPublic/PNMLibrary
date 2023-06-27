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

#include "sls/compute_unit/axdimm_compute_unit.h"

#include "sls/compute_unit/compute_unit.h"

#include "core/device/sls/base.h"
#include "core/device/sls/constants.h"

#include "common/make_error.h"

#include "pnmlib/core/device.h"

#include "pnmlib/common/views.h"

#include <atomic>
#include <cstdint>
#include <type_traits>

namespace {
void poll_register(volatile std::atomic<uint32_t> *reg, uint32_t target) {
  // [TODO: @n-enin] Check if yield can be safely added with AxDIMM hardware
  while (reg->load() != target) {
    ;
  }
}
} // namespace

namespace pnm::sls {

AXDIMMComputeUnit::AXDIMMComputeUnit(device::BaseDevice *device, uint8_t pack)
    : device_(device) {
  const sls::device::BaseDevice::bit_mask ranks_range(pack);
  while (
      !device_->acquire_cunit_for_write_from_range(&cunit_id_, ranks_range)) {
  }

  while (!device_->acquire_cunit_for_read(cunit_id_)) {
  }

  enable();
}

void AXDIMMComputeUnit::disable() {
  device_->control_switch_register(cunit_id_)->store(
      sls::device::BaseDevice::SLS_DISABLE_VALUE);
}

void AXDIMMComputeUnit::enable() {
  device_->control_switch_register(cunit_id_)->store(
      sls::device::BaseDevice::SLS_ENABLE_VALUE);
}

AXDIMMComputeUnit::~AXDIMMComputeUnit() {
  disable();
  device_->release_cunit_for_read(cunit_id_);
  device_->release_cunit_for_write(cunit_id_);
}

void AXDIMMComputeUnit::run() {
  run_async();
  poll(PollType::FINISH);
}

void AXDIMMComputeUnit::run_async() {
  device_->exec_sls_register(cunit_id_)->store(
      sls::device::BaseDevice::SLS_ENABLE_VALUE);
}

void AXDIMMComputeUnit::wait() { poll(PollType::FINISH); }

void AXDIMMComputeUnit::poll(PollType type) {
  switch (type) {
  case PollType::FINISH:
    poll_register(device_->psum_poll_register(cunit_id_),
                  pnm::sls::device::POLL_FINISH_VALUE);
    break;
  default:
    throw pnm::error::make_not_sup(
        "Poll type: {},  device {}", std::underlying_type_t<PollType>(type),
        std::underlying_type_t<Device::Type>(device_->get_device_type()));
  }
}

void AXDIMMComputeUnit::read_buffer(BlockType type,
                                    pnm::common_view<uint8_t> data) {
  Parent::read_buffer(device_, cunit_id_, type, data);
}

void AXDIMMComputeUnit::write_buffer(BlockType type,
                                     pnm::common_view<const uint8_t> data) {
  Parent::write_buffer(device_, cunit_id_, type, data);
}

} // namespace pnm::sls
