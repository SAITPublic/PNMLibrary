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

#ifndef _AXDIMM_SLS_SIMULATOR_H_
#define _AXDIMM_SLS_SIMULATOR_H_

#include "core/device/sls/simulator/base_sls_sim.h"
#include "core/device/sls/simulator_core/axdimm_core_sim.h"

#include <cstddef>
#include <cstdint>

namespace pnm::sls::device {
class AxdimmSimulator : public BaseSlsSimulator {
public:
  AxdimmSimulator(AxdimmSimulatorCore &sim, SLSControlRegister &ctrl_reg);
  ~AxdimmSimulator() override;
  void shift_buffers(uint8_t compute_unit);
  void clear_buffers(uint8_t compute_unit);

private:
  void exec_sls(size_t thread_id) override;

  AxdimmSimulatorCore &sim_;
};
} // namespace pnm::sls::device

#endif
