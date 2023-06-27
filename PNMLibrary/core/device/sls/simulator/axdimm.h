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

#include "core/device/sls/memory_map.h"
#include "core/device/sls/nbuffer_memory.h"
#include "core/device/sls/rank_address.h"

#include "common/compiler_internal.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#ifndef _SLS_AXDIMM_SIMULATOR_H_
#define _SLS_AXDIMM_SIMULATOR_H_

namespace pnm::sls::device {

class AxdimmSimulator {
public:
  explicit AxdimmSimulator(MemMap &mem_map);

  void exec_trace(uint8_t compute_unit);
  void clear_buffers(uint8_t compute_unit);
  void clear_psum_buf(uint8_t compute_unit, uint8_t buf_idx,
                      uint32_t offset = 0);
  void clear_tags_buf(uint8_t compute_unit, uint8_t buf_idx,
                      uint32_t offset = 0);

  pnm::sls::device::InterleavedPointer current_psum_buf(uint8_t compute_unit,
                                                        uint32_t offset) const;
  pnm::sls::device::InterleavedPointer current_tags_buf(uint8_t compute_unit,
                                                        uint32_t offset) const;

  void handle_psum_buf_read(uint8_t compute_unit, uint32_t offset);
  void handle_tags_buf_read(uint8_t compute_unit, uint32_t offset);

private:
  // Map {compute_unit => offset}.
  using OffsetComputeUnitMap = std::vector<uint64_t>;
  // Map {psum buffer index => {compute_unit => offset}}.
  using PsumOffsetBufMap = std::vector<OffsetComputeUnitMap>;
  // Map {tags buffer index => {compute_unit => offset}}.
  using TagsOffsetBufMap = std::vector<OffsetComputeUnitMap>;
  // Map {any buffer index => {compute_unit => offset}} (not owned).
  using OffsetBufMapRef = OffsetComputeUnitMap *;

  void clear_memory(pnm::sls::device::InterleavedPointer ptr, size_t size);
  void clear_buf_common(OffsetBufMapRef min_offsets,
                        OffsetBufMapRef max_offsets, NBufferedMemory &memory,
                        uint64_t buffer_size, uint8_t compute_unit,
                        uint8_t buf_idx, uint32_t offset = 0);
  template <typename T>
  void
  sparse_ft_sum(InterleavedPointer ft_table, InterleavedPointer psum,
                uint64_t bytes_per_idx) NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW;

  void sparse_ft_sum_tagged(InterleavedPointer ft_table,
                            InterleavedPointer psum, InterleavedPointer tags,
                            uint64_t bytes_per_idx)
      NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW;

  void write_reg(uint8_t *reg_addr, uint32_t val);

  std::unique_ptr<NBufferedMemory> psum_memory;
  std::unique_ptr<NBufferedMemory> tags_memory;

  // The maximum and minimum PSUM and TAGS offset to which data has been written
  // in the current trace. The minimum offset is included in the range, but the
  // maximum is not. Since the PSUM write size can change
  // from trace to trace (different modes in the header), and TAGS write size is
  // constant, deriving TAGS offsets from PSUM offsets or vice versa can be
  // difficult, so we just store offset maps for both regions.
  PsumOffsetBufMap min_psum_offsets_;
  PsumOffsetBufMap max_psum_offsets_;
  TagsOffsetBufMap min_tags_offsets_;
  TagsOffsetBufMap max_tags_offsets_;

  // NOTE: This is a hack to share a memory map (owned by a memblockhandler)
  // between the memblockhandler itself and the simulator owned by this
  // memblockhandler. [TODO: MCS23-1393] Find and implement a cleaner solution.
  const MemMap &mem_map_;
};

} // namespace pnm::sls::device

#endif // _SLS_AXDIMM_SIMULATOR_H_
