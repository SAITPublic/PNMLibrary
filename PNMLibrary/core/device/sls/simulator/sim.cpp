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

#include "sim.h"

#include "core/device/simulator/base_simulator.h"
#include "core/device/sls/base.h"
#include "core/device/sls/memblockhandler/axdimm/sim.h"
#include "core/device/sls/memblockhandler/cxl/sim.h"
#include "core/device/sls/simulator/axdimm_sim.h"
#include "core/device/sls/simulator/base_sls_sim.h"
#include "core/device/sls/simulator/cxl_sim.h"

#include "common/make_error.h"
#include "common/topology_constants.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <type_traits>
#include <vector>

namespace pnm::sls::device {

SimulatorDevice::SimulatorDevice(BusType type,
                                 const std::filesystem::path &data_path)
    : BaseDevice(type, data_path) {
  if constexpr (PNM_PLATFORM == HARDWARE) {
    throw pnm::error::make_inval(
        "SimulatorDevice cannot be created in hardware mode.");
  }
  setup_sim();
}

BaseAsyncSimulator &SimulatorDevice::get_sim() {
  // We want to launch thread pool only in runner
  // to prevent usage of parent threads by child processes.
  std::call_once(sim_start_flag_, [this]() { sim_->start(); });
  return *sim_;
}

void SimulatorDevice::setup_sim() {
  SLSControlRegister ctrl_reg;
  ctrl_reg.exec_sls_reg.resize(topo().NumOfCUnits);
  ctrl_reg.psum_pol_reg.resize(topo().NumOfCUnits);
  for (uint32_t i = 0; i < topo().NumOfCUnits; ++i) {
    ctrl_reg.exec_sls_reg[i] = exec_sls_register(i);
    ctrl_reg.psum_pol_reg[i] = psum_poll_register(i);
  }
  switch (bus_type_) {
  case BusType::AXDIMM: {
    sim_ =
        create_sim<AxdimmSimulatorMemBlockHandler, AxdimmSimulator>(ctrl_reg);
    return;
  }
  case BusType::CXL: {
    sim_ = create_sim<CxlSimulatorMemBlockHandler, CxlSimulator>(ctrl_reg);
    return;
  }
  }

  throw pnm::error::make_not_sup("Device type {} for SLS.",
                                 std::underlying_type_t<BusType>(bus_type_));
}

} // namespace pnm::sls::device
