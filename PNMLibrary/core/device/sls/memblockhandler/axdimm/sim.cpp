/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or
 * computer language in any form
 * by any means, electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung
 * Electronics.
 */

#include "sim.h"

#include "core/device/sls/utils/memory_map.h"
#include "core/device/sls/utils/rank_address.h"

#include <linux/sls_resources.h>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace pnm::sls::device {

// `mem_addr` is unused because the simulator itself has access to the memory
// map, so identification by compute unit is sufficient.
void AxdimmSimulatorPsumReader::operator()(
    [[maybe_unused]] const memAddr &mem_addr, uint8_t compute_unit,
    [[maybe_unused]] sls_mem_blocks_e block_type, uint32_t offset,
    uint8_t *buf_out, size_t read_size) {
  assert(block_type == SLS_BLOCK_PSUM);

  const auto psum_buf = simulator_->current_psum_buf(compute_unit, offset);
  memcpy_interleaved(psum_buf, read_size, buf_out);

  simulator_->handle_psum_buf_read(compute_unit, offset);
}

void AxdimmSimulatorTagsReader::operator()(
    [[maybe_unused]] const memAddr &mem_addr, uint8_t compute_unit,
    [[maybe_unused]] sls_mem_blocks_e block_type, uint32_t offset,
    uint8_t *buf_out, size_t read_size) {
  assert(block_type == SLS_BLOCK_TAGS);

  const auto tags_buf = simulator_->current_tags_buf(compute_unit, offset);
  memcpy_interleaved(tags_buf, read_size, buf_out);

  simulator_->handle_tags_buf_read(compute_unit, offset);
}

/*
  The following methods are added for gdb usage only. due to the fact that
  devices memory is not accessible directly from gdb, for example trying to
  execute the following command 'x/32 [psum address]' returns
  'cannot access memory' error. this methods can be used as follows:
  - set breakpoint somewhere in AxdimmSimulator scope
  - while halted on breakpoint execute command:
     'p/x this->get_mem_val(compute_unit, memory type, offset)'
  where offset should be given with respect to interleaving
*/

[[maybe_unused]] uint32_t
AxdimmSimulatorMemBlockHandler::get_mem_val(uint8_t compute_unit, uint32_t type,
                                            uint64_t offset) {
  const uint32_t *pmem =
      reinterpret_cast<uint32_t *>(mem_map()[compute_unit].addr[type]);
  return pmem[offset / sizeof(uint32_t)];
}

[[maybe_unused]] uint64_t
AxdimmSimulatorMemBlockHandler::get_mem_addr(uint8_t compute_unit,
                                             uint32_t type, uint64_t offset) {
  const uint32_t *pmem =
      reinterpret_cast<uint32_t *>(mem_map()[compute_unit].addr[type]);
  return reinterpret_cast<uint64_t>(&pmem[offset / sizeof(uint32_t)]);
}

[[maybe_unused]] void
AxdimmSimulatorMemBlockHandler::write_reg(uint8_t *reg_addr, uint32_t val) {
  *reinterpret_cast<volatile uint32_t *>(reg_addr) = val;
}

} // namespace pnm::sls::device
