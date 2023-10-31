// base.cpp - sls device base operations -----------------------//

/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
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

#include "base.h"

#include "core/device/sls/memblockhandler/base.h"
#include "core/device/sls/simulator/sim.h"
#include "core/device/sls/utils/constants.h"

#include "common/compiler_internal.h"
#include "common/log.h"
#include "common/make_error.h"
#include "common/timer.h"
#include "common/topology_constants.h"

#include "pnmlib/sls/control.h"

#include "pnmlib/core/device.h"

#include "pnmlib/common/128bit_math.h"
#include "pnmlib/common/error.h"
#include "pnmlib/common/rowwise_view.h"

#include <linux/sls_resources.h>

#include <fcntl.h>
#include <sys/ioctl.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <type_traits>
#include <vector>

namespace pnm::sls::device {

namespace {

inline void check_memory_access([[maybe_unused]] uint8_t compute_unit) {
  assert(compute_unit < topo().NumOfCUnits);
}

auto &cunit_tmp_storages(int compute_unit) {
  static std::vector<std::vector<uint8_t>> buf(
      topo().NumOfCUnits, std::vector<uint8_t>(topo().TagsBufSize));
  return buf[compute_unit];
}

} // namespace

BaseDevice::BaseDevice(BusType type, const std::filesystem::path &data_path)
    : control_fd_(SLS_RESOURCE_PATH, O_RDWR), data_fd_(data_path, O_RDWR),
      tsan_mutexes_(topo().NumOfCUnits),
      mem_block_handler_(ISlsMemBlockHandler::create(type)), bus_type_(type) {
  if (topo().NumOfInstBuf > 2) {
    throw pnm::error::NotSupported("Invalid number of instruction buffers.");
  }

  if (topo().NumOfPSumBuf > 2) {
    throw pnm::error::NotSupported("Invalid number of psum buffers.");
  }

  const auto mem_info = control_.get_mem_info();
  mem_block_handler_->init(mem_info, *data_fd_);

  pnm::log::info("sls::BaseDevice is initialized.");
}

uint32_t *BaseDevice::psum_poll_register(uint8_t compute_unit) const {
  check_memory_access(compute_unit);
  // The writer side is SLS hardware, don't mess up with ordering
  return static_cast<uint32_t *>(mem_block_handler_->get_mem_block_ptr(
      SLS_BLOCK_CFGR, compute_unit, topo().RegPolling + sizeof(uint32_t)));
}

uint32_t *BaseDevice::exec_sls_register(uint8_t compute_unit) {
  return static_cast<uint32_t *>(mem_block_handler_->get_mem_block_ptr(
      SLS_BLOCK_CFGR, compute_unit, topo().RegSlsExec));
}

uint32_t *BaseDevice::control_switch_register(uint8_t compute_unit) {
  return static_cast<uint32_t *>(mem_block_handler_->get_mem_block_ptr(
      SLS_BLOCK_CFGR, compute_unit, topo().RegSlsEn));
}

uint8_t BaseDevice::acquire_idle_cunit_from_range(cunit cunit_mask) const {

  const unsigned long mask = cunit_mask.to_ulong();
  int res = ioctl(*control_fd_, GET_CUNIT, mask);

  if (res < 0) {
    res = topo().NumOfCUnits;
  }

  return res;
}

bool BaseDevice::acquire_cunit_from_range(uint8_t *compute_unit,
                                          cunit cunit_mask) {
  const auto cunit_tmp = acquire_idle_cunit_from_range(cunit_mask);

  if (cunit_tmp == topo().NumOfCUnits) {
    return false;
  }
  *compute_unit = cunit_tmp;

  if constexpr (TSAN) {
    tsan_mutexes_[cunit_tmp].lock();
  }

  pnm::log::debug("Selected compute unit {:d}", cunit_tmp);
  return true;
}

void BaseDevice::release_cunit(uint8_t compute_unit) const {
  if constexpr (TSAN) {
    tsan_mutexes_[compute_unit].unlock();
  }

  const int res = ioctl(*control_fd_, RELEASE_CUNIT, compute_unit);
  if (res < 0) {
    PNM_LOG_ERROR("Failed to release compute unit.");
    throw pnm::error::Failure("ioctl RELEASE_CUNIT.");
  }

  pnm::log::debug("Released compute unit {}", compute_unit);
}

std::unique_ptr<BaseDevice> BaseDevice::make_device() {
  const auto bus_type = topo().Bus;

  switch (bus_type) {
  case BusType::AXDIMM:
  case BusType::CXL: {
    const auto mem_path = get_device_path();
    return ((PNM_PLATFORM == HARDWARE)
                ? std::make_unique<BaseDevice>(bus_type, mem_path)
                : std::make_unique<SimulatorDevice>(bus_type, mem_path));
  }
  }

  throw pnm::error::make_not_sup(
      "Device type {} for SLS.",
      std::underlying_type_t<Device::Type>(bus_type));
}

void BaseDevice::read_and_reset_tags(uint8_t compute_unit, uint8_t *tags_out,
                                     size_t read_size) {
  MEASURE_TIME();

  static constexpr auto single_tag_size = sizeof(pnm::types::uint128_t);
  const auto aligned_tags_buffer_size =
      read_size / single_tag_size * topo().AlignedTagSize;

  auto &tags_tmp = cunit_tmp_storages(compute_unit);
  mem_block_handler_->read_block(SLS_BLOCK_TAGS, compute_unit, 0,
                                 tags_tmp.data(), aligned_tags_buffer_size);

  const auto tags_buffer_view = pnm::views::make_rowwise_view(
      tags_tmp.data(), tags_tmp.data() + aligned_tags_buffer_size,
      topo().AlignedTagSize, 0, topo().AlignedTagSize - single_tag_size);
  std::for_each(tags_buffer_view.begin(), tags_buffer_view.end(),
                [&tags_out](auto tag) {
                  tags_out = std::copy(tag.begin(), tag.end(), tags_out);
                });
}

void BaseDevice::reset_impl(Device::ResetOptions options) {
  switch (options) {
  case ResetOptions::SoftwareOnly:
  case ResetOptions::SoftwareHardware:
    return control_.sysfs_reset();
  case ResetOptions::HardwareOnly:
    break;
  }

  throw pnm::error::make_not_sup("Reset option.");
}

} // namespace pnm::sls::device
