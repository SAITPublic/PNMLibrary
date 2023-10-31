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

#ifndef _SLS_CXL_SIM_COMPUTE_DEVICE_H_
#define _SLS_CXL_SIM_COMPUTE_DEVICE_H_

#include "core/device/sls/base.h"
#include "core/device/sls/simulator/compute_device.h"

#include <cstdint>

namespace pnm::sls::device {

//! @brief The CXL simulator device. Implements specific behaviour for CXL SLS.
class CxlSimulatorComputeDevice : public SimulatorComputeDevice {
public:
  CxlSimulatorComputeDevice(BaseDevice *device);

  void write_control_switch_register(uint8_t compute_unit, uint32_t val);
};

} // namespace pnm::sls::device

#endif
