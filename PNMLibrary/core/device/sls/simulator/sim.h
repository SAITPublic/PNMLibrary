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

#ifndef _SLS_SIMULATOR_DEVICE_H_
#define _SLS_SIMULATOR_DEVICE_H_

#include "core/device/simulator/base_simulator.h"
#include "core/device/sls/base.h"
#include "core/device/sls/simulator/base_sls_sim.h"

#include "common/topology_constants.h"

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <mutex>

namespace pnm::sls::device {

class SimulatorDevice : public BaseDevice {
public:
  SimulatorDevice(BusType type, const std::filesystem::path &data_path);

  BaseAsyncSimulator &get_sim();

private:
  template <typename SimulatorMemBlockHandlerType, typename SimulatorType>
  std::unique_ptr<BaseAsyncSimulator> create_sim(SLSControlRegister &ctrl_reg) {
    auto *memblockhandler =
        static_cast<SimulatorMemBlockHandlerType *>(mem_block_handler_.get());
    return std::make_unique<SimulatorType>(memblockhandler->simulator(),
                                           ctrl_reg);
  }
  void setup_sim();

  std::unique_ptr<BaseAsyncSimulator> sim_;
  std::once_flag sim_start_flag_;
};

} // namespace pnm::sls::device

#endif /* _SLS_SIMULATOR_DEVICE_H_ */
