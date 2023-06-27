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

#ifndef _SLS_CXL_SIMULATOR_H_
#define _SLS_CXL_SIMULATOR_H_

#include "core/device/sls/memory_map.h"

#include "common/compiler_internal.h"

#include <cstdint>

namespace pnm::sls::device {

class CXLSLSSimulator {
public:
  explicit CXLSLSSimulator(const MemMap &mem_map);

  void exec_trace(uint8_t channel);
  void clear_buffers(uint8_t channel);

private:
  template <typename T>
  void sparse_ft_sum(const uint8_t *channel_base, uint64_t channel_addr,
                     uint32_t output_idx, uint8_t *psum_buf,
                     uint64_t bytes_per_idx);
  void sparse_ft_sum_tagged(const uint8_t *channel_base, uint64_t channel_addr,
                            uint32_t output_idx, uint8_t *psum_buf,
                            uint8_t *tags_buf, uint64_t bytes_per_idx)
      NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW;
  void write_reg(uint8_t *reg_addr, uint32_t val);

  // NOTE: This is a hack to share a memory map (owned by a memblockhandler)
  // between the memblockhandler itself and the simulator owned by this
  // memblockhandler. [TODO: @a.korzun] Find and implement a cleaner solution.
  const MemMap &mem_map_;
};

} // namespace pnm::sls::device

#endif //_SLS_CXL_SIMULATOR_H_
