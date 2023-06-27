// simulator.cpp - imdb simulator specific device ops --------------//

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

#include "simulator.h"

#include "core/device/imdb/base.h"
#include "core/device/imdb/simulator_core.h"

#include "common/log.h"

#include "pnmlib/imdb/libimdb.h"

#include <cstdint>
#include <memory>

namespace pnm::imdb::device {

SimulatorDevice::SimulatorDevice()
    : BaseDevice(IMDB_CSR_DEVICE_SIM, IMDB_DEVMEMORY_PATH_SIM),
      sim_memory_(this->get_devmem_fd(), control_.memory_size(),
                  "IMDB memory") {
  sim_ = std::make_unique<SimulatorCore>(
      regs_, static_cast<uint8_t *>(sim_memory_.data()));
  pnm::log::info("IMDB SimulatorDevice is initialized.");
}

SimulatorDevice::~SimulatorDevice() {
  pnm::log::info("IMDB SimulatorDevice is deinitialized.");
}

uint8_t SimulatorDevice::lock_thread_impl() {
  const uint8_t thread_id = BaseDevice::lock_thread_impl();
  sim_->enable_thread(thread_id);
  return thread_id;
}

void SimulatorDevice::release_thread_impl(uint8_t thread_id) {
  sim_->disable_thread(thread_id);
  BaseDevice::release_thread_impl(thread_id);
}

} // namespace pnm::imdb::device
