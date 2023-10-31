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

#include "core/device/sls/simulator/axdimm_compute_device.h"

#include "core/device/sls/base.h"
#include "core/device/sls/simulator/axdimm_sim.h"
#include "core/device/sls/simulator/compute_device.h"
#include "core/device/sls/simulator/sim.h"
#include "core/device/sls/utils/constants.h"

#include <cassert>
#include <cstdint>

namespace pnm::sls::device {

AxdimmSimulatorComputeDevice::AxdimmSimulatorComputeDevice(BaseDevice *device)
    : SimulatorComputeDevice(device) {
  // Since the SimulatorDevice is in the arguments, we can be sure that the
  // static_cast is correct
  sim_ = dynamic_cast<AxdimmSimulator *>(
      &(static_cast<SimulatorDevice *>(device_)->get_sim()));
  assert(sim_);
}

void AxdimmSimulatorComputeDevice::write_exec_sls_register(uint8_t compute_unit,
                                                           uint32_t val) {
  if (val == SLS_ENABLE_VALUE) {
    sim_->shift_buffers(compute_unit);
  }
  SimulatorComputeDevice::write_exec_sls_register(compute_unit, val);
}

void AxdimmSimulatorComputeDevice::write_control_switch_register(
    uint8_t compute_unit, uint32_t val) {
  if (val == SLS_ENABLE_VALUE) {
    sim_->clear_buffers(compute_unit);
  }
  SimulatorComputeDevice::write_control_switch_register(compute_unit, val);
}

} // namespace pnm::sls::device
