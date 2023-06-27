/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or
 * computer language in any form
 * by any means, electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung
 * Electronics.
 */

#include "pnmlib/core/device.h"

#include "core/device/sls/base.h"

#include "pnmlib/core/sls_cunit_info.h"
#include "pnmlib/core/sls_device.h"

#include "pnmlib/common/error.h"

#include <cstddef>
#include <cstdint>

// NOTE: Put implementations of ALL methods in .cpp because in header we don't
// have full access to BaseDevice, as we only-forward declare it there, so
// the default-generated methods fail to link

SlsDevice::SlsDevice() noexcept : device_{nullptr} {}
SlsDevice::SlsDevice(SlsDevice &&) noexcept = default;
SlsDevice &SlsDevice::operator=(SlsDevice &&) noexcept = default;
SlsDevice::~SlsDevice() = default;

void SlsDevice::open(pnm::Device::Type dev_type) {
  if (is_open()) {
    throw pnm::error::InvalidArguments(
        "Trying to open an already opened device.");
  }

  device_ = pnm::sls::device::BaseDevice::make_device(dev_type);
}

void SlsDevice::close() {
  if (!is_open()) {
    throw pnm::error::InvalidArguments(
        "Trying to close an already closed device.");
  }

  device_ = nullptr;
}

bool SlsDevice::is_open() const noexcept { return device_ != nullptr; }

SlsDevice SlsDevice::make(pnm::Device::Type dev_type) {
  SlsDevice device;
  device.open(dev_type);
  return device;
}

size_t SlsDevice::base_memory_size() const noexcept {
  return device_->get_base_memory_size();
}

uint64_t SlsDevice::compute_unit_info(uint8_t compute_unit,
                                      SlsComputeUnitInfo key) const {
  return device_->get_compute_unit_info(compute_unit, key);
}

void SlsDevice::reset() { device_->reset(); }

uint64_t SlsDevice::acquisition_timeout() const {
  return device_->get_acquisition_timeout();
}

void SlsDevice::set_acquisition_timeout(uint64_t value) {
  device_->set_acquisition_timeout(value);
}

uint64_t SlsDevice::leaked_count() const { return device_->get_leaked(); }

bool SlsDevice::resource_cleanup() const {
  // [TODO: s.motov] remove magic number
  return device_->get_resource_cleanup() != 0;
}

void SlsDevice::set_resource_cleanup(bool value) {
  device_->set_resource_cleanup(value);
}

pnm::sls::device::BaseDevice *SlsDevice::internal() noexcept {
  return device_.get();
}

const pnm::sls::device::BaseDevice *SlsDevice::internal() const noexcept {
  return device_.get();
}
