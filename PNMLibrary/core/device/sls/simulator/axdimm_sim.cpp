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

#include "core/device/sls/simulator/axdimm_sim.h"

#include "core/device/sls/simulator/base_sls_sim.h"

#include <cstddef>
#include <cstdint>

namespace pnm::sls::device {

AxdimmSimulator::AxdimmSimulator(AxdimmSimulatorCore &sim,
                                 SLSControlRegister &ctrl_reg)
    : BaseSlsSimulator(ctrl_reg), sim_(sim) {}

AxdimmSimulator::~AxdimmSimulator() { stop(); }

void AxdimmSimulator::exec_sls(size_t thread_id) { sim_.exec_trace(thread_id); }

void AxdimmSimulator::shift_buffers(uint8_t compute_unit) {
  sim_.shift_buffers(compute_unit);
}

void AxdimmSimulator::clear_buffers(uint8_t compute_unit) {
  sim_.clear_buffers(compute_unit);
}

} // namespace pnm::sls::device
