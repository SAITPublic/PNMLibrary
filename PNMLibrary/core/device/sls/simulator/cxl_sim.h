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

#ifndef _CXL_SLS_SIMULATOR_H_
#define _CXL_SLS_SIMULATOR_H_

#include "core/device/sls/simulator/base_sls_sim.h"
#include "core/device/sls/simulator_core/cxl_core_sim.h"

#include <cstddef>

namespace pnm::sls::device {
class CxlSimulator : public BaseSlsSimulator {
public:
  CxlSimulator(CxlSimulatorCore &sim, SLSControlRegister &ctrl_reg);
  ~CxlSimulator() override;

private:
  void exec_sls(size_t thread_id) override;

  CxlSimulatorCore &sim_;
};
} // namespace pnm::sls::device

#endif
