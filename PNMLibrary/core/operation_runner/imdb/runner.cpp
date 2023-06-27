/*
 *  Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 *  This software is proprietary of Samsung Electronics.
 *  No part of this software, either material or conceptual may be copied or
 *  distributed, transmitted, transcribed, stored in a retrieval system or
 *  translated into any human or computer language in any form by any means,
 *  electronic, mechanical, manual or otherwise, or disclosed to third parties
 *  without the express written permission of Samsung Electronics.
 */

#include "runner.h"

#include "imdb/operation/scan.h"

#include "core/device/imdb/base.h"
#include "core/operation/internal.h"

#include "pnmlib/imdb/libimdb.h"

#include "pnmlib/core/device.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/misc_utils.h"

#include <cstdint>
#include <thread>

namespace pnm::imdb {

void Runner::run_operator(ScanOperation &operation) {
  const uint8_t thread_id = device_->lock_thread();
  auto *CSR = device_->get_csr(thread_id);

  operation.write_registers(CSR);

  run_and_wait(CSR);

  operation.read_registers(CSR);

  device_->release_thread(thread_id);
}

Runner::Runner(Device *device) {
  if (device == nullptr) {
    throw pnm::error::InvalidArguments("Expected device.");
  }

  device_ = device->as<device::BaseDevice>();
}

void Runner::run_and_wait(volatile ThreadCSR *CSR) {
  auto *status = utils::as_vatomic<uint32_t>(&CSR->THREAD_STATUS);

  // Workaround to be sure that we will start waiting the result.
  // [TODO: @y-lavrinenko] Remove when simulator core will use cv.
  if constexpr (PNM_PLATFORM != HARDWARE) {
    status->store(THREAD_STATUS_BUSY);
  }

  auto *control = utils::as_vatomic<uint32_t>(&CSR->THREAD_CTRL);
  control->store(control->load() | THREAD_CTRL_START);

  while (status->load() == THREAD_STATUS_BUSY) {
    std::this_thread::yield();
  }
}

void Runner::run_impl(pnm::InternalOperator &op) {
  auto &casted_op = static_cast<ScanOperation &>(op);
  run_operator(casted_op);
}

} // namespace pnm::imdb
