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

#ifndef _SLS_CXL_DEVICE_H_

#include "sls/compute_unit/compute_unit.h"

#include "core/device/sls/base.h"

#include "pnmlib/common/views.h"

#include <cstdint>

namespace pnm::sls {

class CXLComputeUnit : public ComputeUnit<CXLComputeUnit> {
  using Parent = ComputeUnit<CXLComputeUnit>;

public:
  CXLComputeUnit(device::BaseDevice *device, uint8_t pack);
  ~CXLComputeUnit();

  uint8_t id() const { return cunit_id_; }

  void enable();
  void disable();
  void run();
  void run_async();
  void wait();
  void poll(PollType type);

  void read_buffer(BlockType type, pnm::common_view<uint8_t> data);
  void write_buffer(BlockType type, pnm::common_view<const uint8_t> data);

private:
  device::BaseDevice *device_{};
  uint8_t cunit_id_;
};

} // namespace pnm::sls

#define _SLS_CXL_DEVICE_H_
#endif
