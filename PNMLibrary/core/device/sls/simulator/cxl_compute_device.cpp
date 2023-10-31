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

#include "core/device/sls/simulator/cxl_compute_device.h"

#include "core/device/sls/base.h"
#include "core/device/sls/simulator/compute_device.h"
#include "core/device/sls/utils/constants.h"

#include <cstdint>

namespace pnm::sls::device {

CxlSimulatorComputeDevice::CxlSimulatorComputeDevice(BaseDevice *device)
    : SimulatorComputeDevice(device) {}

void CxlSimulatorComputeDevice::write_control_switch_register(
    uint8_t compute_unit, uint32_t val) {
  // Hardware sets cs register to ENABLE after DISABLE write
  if (val == SLS_DISABLE_VALUE) {
    SimulatorComputeDevice::write_control_switch_register(compute_unit,
                                                          SLS_ENABLE_VALUE);
    // Hardware sets cs register to CONTROL_SWITCHED after ENABLE write
    // There's polling for this value after enable write in compute unit
  } else if (val == SLS_ENABLE_VALUE) {
    SimulatorComputeDevice::write_control_switch_register(
        compute_unit, SLS_CONTROL_SWITCHED_VALUE);
  } else {
    SimulatorComputeDevice::write_control_switch_register(compute_unit, val);
  }
}

} // namespace pnm::sls::device
