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

#include "sim.h"

#include <cstdint>

namespace pnm::sls::device {

/*
  The following methods are added for gdb usage only. due to the fact that
  devices memory is not accessible directly from gdb, for example trying to
  execute the following command 'x/32 [psum address]' returns
  'cannot access memory' error. this methods can be used as follows:
  - set breakpoint somewhere in CXLSimulator scope
  - while halted on breakpoint execute command:
     'p/x this->get_mem_val(compute_unit, memory type, offset)'
  where offset should be given with respect to interleaving
*/

[[maybe_unused]] uint32_t
CxlSimulatorMemBlockHandler::get_mem_val(uint8_t compute_unit, uint32_t type,
                                         uint64_t offset) {
  const uint32_t *pmem =
      reinterpret_cast<uint32_t *>(mem_map()[compute_unit].addr[type]);
  return pmem[offset / sizeof(uint32_t)];
}

[[maybe_unused]] uint64_t
CxlSimulatorMemBlockHandler::get_mem_addr(uint8_t compute_unit, uint32_t type,
                                          uint64_t offset) {
  const uint32_t *pmem =
      reinterpret_cast<uint32_t *>(mem_map()[compute_unit].addr[type]);
  return reinterpret_cast<uint64_t>(&pmem[offset / sizeof(uint32_t)]);
}

[[maybe_unused]] void CxlSimulatorMemBlockHandler::write_reg(uint8_t *reg_addr,
                                                             uint32_t val) {
  *reinterpret_cast<volatile uint32_t *>(reg_addr) = val;
}

} // namespace pnm::sls::device
