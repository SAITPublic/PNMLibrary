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

#ifndef _SLS_CXL_SIM_MEMBLOCKHANDLER_H_
#define _SLS_CXL_SIM_MEMBLOCKHANDLER_H_

#include "base.h"

#include "core/device/sls/memblockhandler/block_ops.h"
#include "core/device/sls/simulator/cxl.h"

#include <cstdint>

namespace pnm::sls::device {

class CXLSimulatorMemBlockHandler final
    : public CXLMemBlockHandler<CXLSimulatorMemBlockHandler> {
public:
  CXLSimulatorMemBlockHandler() : simulator_(mem_map()) {}

  // [TODO: @a.korzun] this is a temporary hack to support simulator method
  // calls from the outside of the memblockhandler. Existing code should be
  // reworked and this method should then be removed from the public API.
  CXLSLSSimulator &simulator() noexcept { return simulator_; };

  auto psum_reader() const { return RawBlockReader{}; };
  auto tags_reader() const { return RawBlockReader{}; };

private:
  // For debug
  uint32_t get_mem_val(uint8_t compute_unit, uint32_t type, uint64_t offset);
  uint64_t get_mem_addr(uint8_t compute_unit, uint32_t type, uint64_t offset);
  void write_reg(uint8_t *reg_addr, uint32_t val);

  CXLSLSSimulator simulator_;
};

} // namespace pnm::sls::device

#endif //_SLS_CXL_SIM_MEMBLOCKHANDLER_H_
