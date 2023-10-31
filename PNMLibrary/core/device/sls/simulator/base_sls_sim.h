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

#ifndef _BASE_SLS_SIMULATOR_H_
#define _BASE_SLS_SIMULATOR_H_

#include "core/device/simulator/base_simulator.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace pnm::sls::device {

struct SLSControlRegister {
  std::vector<uint32_t *> exec_sls_reg;
  std::vector<uint32_t *> psum_pol_reg;
};

class BaseSlsSimulator : public BaseAsyncSimulator {
public:
  BaseSlsSimulator(SLSControlRegister &ctrl_reg);

protected:
  void worker_job(size_t thread_id) override;
  virtual void exec_sls(size_t thread_id) = 0;

private:
  SLSControlRegister ctrl_reg_;
};

} // namespace pnm::sls::device

#endif
