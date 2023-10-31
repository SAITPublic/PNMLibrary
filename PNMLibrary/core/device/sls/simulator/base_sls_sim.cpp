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

#include "core/device/sls/simulator/base_sls_sim.h"

#include "core/device/simulator/base_simulator.h"
#include "core/device/sls/utils/constants.h"

#include "common/topology_constants.h"

#include "pnmlib/common/misc_utils.h"

#include <cstddef>
#include <cstdint>
#include <thread>
#include <utility>
#include <vector>

namespace pnm::sls::device {

BaseSlsSimulator::BaseSlsSimulator(SLSControlRegister &ctrl_reg)
    : BaseAsyncSimulator(topo().NumOfCUnits), ctrl_reg_(std::move(ctrl_reg)) {}

void BaseSlsSimulator::worker_job(size_t thread_id) {
  auto *exec_sls_atomic_reg =
      pnm::utils::as_vatomic<uint32_t>(ctrl_reg_.exec_sls_reg[thread_id]);
  auto *psum_poll_atomic_reg =
      pnm::utils::as_vatomic<uint32_t>(ctrl_reg_.psum_pol_reg[thread_id]);
  while (!should_stop(thread_id)) {
    // Wait to launch sls operation
    if (exec_sls_atomic_reg->load() != SLS_ENABLE_VALUE) {
      std::this_thread::yield();
      continue;
    }

    exec_sls(thread_id);

    exec_sls_atomic_reg->store(SLS_DISABLE_VALUE);
    psum_poll_atomic_reg->store(POLL_FINISH_VALUE);
  }
}

} // namespace pnm::sls::device
