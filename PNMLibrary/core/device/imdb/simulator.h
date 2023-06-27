// core/device/imdb/simulator.h - imdb simulator device ops *C++*//

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

#ifndef _IMDB_SIMULATOR_DEVICE_H_
#define _IMDB_SIMULATOR_DEVICE_H_

#include "base.h"
#include "simulator_core.h"

#include "common/mapped_file.h"

#include <cstdint>
#include <memory>

namespace pnm::imdb::device {

class SimulatorDevice final : public BaseDevice {
public:
  SimulatorDevice();
  ~SimulatorDevice() override;

protected:
  uint8_t lock_thread_impl() override;
  void release_thread_impl(uint8_t thread_id) override;

private:
  pnm::utils::MappedData sim_memory_;
  std::unique_ptr<SimulatorCore> sim_{};
};

} // namespace pnm::imdb::device

#endif /* _IMDB_SIMULATOR_DEVICE_H_ */
