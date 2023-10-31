//=----------- base.cpp - IMDB device common ops -*- C++-*-----=//

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

#include "base.h"

#include "hardware.h"
#include "reg_mem.h"
#include "simulator.h"

#include "common/compiler_internal.h"
#include "common/error_message.h"
#include "common/make_error.h"
#include "common/memory/atomic.h"
#include "common/topology_constants.h"

#include "pnmlib/imdb/libimdb.h"

#include "pnmlib/core/device.h"

#include <fmt/core.h>

#include <linux/imdb_resources.h>

#include <fcntl.h>
#include <sys/ioctl.h>

#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <thread>
#include <type_traits>

namespace pnm::imdb::device {

using pnm::imdb::device::topo;

namespace {
template <typename T> T *drop_volatile(volatile T *ptr) {
  return const_cast<T *>(ptr);
}
} // namespace

std::unique_ptr<BaseDevice> BaseDevice::make_device() {
  using ConcreteDeviceType =
      std::conditional_t<PNM_PLATFORM != HARDWARE, SimulatorDevice,
                         HardwareDevice>;
  return std::make_unique<ConcreteDeviceType>();
}

void BaseDevice::reset_impl(ResetOptions options) {
  switch (options) {
  case ResetOptions::SoftwareOnly:
    return software_reset();
  case ResetOptions::HardwareOnly:
    return hardware_reset();
  case ResetOptions::SoftwareHardware:
    software_reset();
    hardware_reset();
    return;
  }

  throw pnm::error::make_not_sup("Reset option.");
}

void BaseDevice::dump_thread_csr(uint8_t thread_id) const {
  // [TODO: @e-kutovoi] Print this in a structured manner
  assert(thread_id < topo().NumOfThreads);
  for (auto reg : regs_as_ints(drop_volatile(regs_.thread[thread_id]))) {
    fmt::print("{} ", reg);
  }

  fmt::print("\n");
}

volatile ThreadCSR *BaseDevice::get_csr(uint8_t thread_id) {
  assert(thread_id < topo().NumOfThreads);
  return regs_.thread[thread_id];
}

const volatile ThreadCSR *BaseDevice::get_csr(uint8_t thread_id) const {
  assert(thread_id < topo().NumOfThreads);
  return regs_.thread[thread_id];
}

BaseDevice::BaseDevice(const std::filesystem::path &devcontrol_path,
                       const std::filesystem::path &devmem_path)
    : devcontrol_(devcontrol_path, REG_AREA_SIZE), devmem_(devmem_path, O_RDWR),
      resmanager_(IMDB_RESOURCE_DEVICE_PATH, O_RDWR) {
  regs_.common = get_regs_section<CommonCSR>(0);
  regs_.status = get_regs_section<StatusCSR>(THREAD_STATUS_OFFSET);
  regs_.thread = std::shared_ptr<volatile ThreadCSR *[]>(
      new volatile ThreadCSR *[topo().NumOfThreads]);

  for (std::size_t thread_id = 0; thread_id < topo().NumOfThreads;
       ++thread_id) {
    auto offset = THREAD_CSR_BASE_OFFSET + THREAD_CSR_SIZE * thread_id;
    regs_.thread[thread_id] = get_regs_section<ThreadCSR>(offset);
  }

  tsan_mutexes_.resize(topo().NumOfThreads);
}

uint8_t BaseDevice::lock_thread() { return lock_thread_impl(); }

void BaseDevice::release_thread(uint8_t thread_id) {
  release_thread_impl(thread_id);
}

uint8_t BaseDevice::lock_thread_impl() {
  const int rc = ioctl(resmanager_.get(), IMDB_IOCTL_GET_THREAD);

  if (rc < 0) {
    pnm::error::throw_from_errno(errno, pnm::error::ioctl_from_errno);
  }

  const auto thread_id = static_cast<uint8_t>(rc);

  if constexpr (TSAN) {
    tsan_mutexes_[thread_id].lock();
  }

  return thread_id;
}

void BaseDevice::release_thread_impl(uint8_t thread_id) {
  if constexpr (TSAN) {
    tsan_mutexes_[thread_id].unlock();
  }

  const int rc = ioctl(resmanager_.get(), IMDB_IOCTL_RELEASE_THREAD, thread_id);

  if (rc < 0) {
    pnm::error::throw_from_errno(errno, pnm::error::ioctl_from_errno);
  }
}

void BaseDevice::software_reset() { control_.reset(); }

void BaseDevice::hardware_reset() const {
  pnm::memory::hw_atomic_store(&regs_.common->IMDB_RESET, IMDB_RESET_ENABLE);

  // [TODO: @e-kutovoi] Investigate whether we need this
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  pnm::memory::hw_atomic_store(&regs_.common->IMDB_RESET, IMDB_RESET_DISABLE);
}

} // namespace pnm::imdb::device
