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

#include "axdimm_core_sim.h"

#include "core/device/sls/utils/constants.h"
#include "core/device/sls/utils/inst_generator.h"
#include "core/device/sls/utils/memory_map.h"
#include "core/device/sls/utils/nbuffer_memory.h"
#include "core/device/sls/utils/rank_address.h"

#include "common/compiler_internal.h"
#include "common/log.h"
#include "common/topology_constants.h"
#include "hw/pnm_hw_instr_constants.h"

#include "pnmlib/common/128bit_math.h"

#include <linux/sls_resources.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace {

inline uint64_t get_bytes_per_idx(uint64_t mode) {
  assert(mode <= 0x02);
  return 64 << mode;
}

// Returns the smallest number that is aligned (num % alignment == 0) and is no
// less than the provided value.
template <typename T> inline constexpr T align_up(T value, T alignment) {
  if (value % alignment == 0) {
    return value;
  }
  return value + alignment - value % alignment;
}

// Returns the largest number that is aligned (num % alignment == 0) and is no
// greater than the provided value.
template <typename T> inline constexpr T align_down(T value, T alignment) {
  return value - value % alignment;
}

} // namespace

namespace pnm::sls::device {

AxdimmSimulatorCore::AxdimmSimulatorCore(MemMap &mem_map)
    : psum_memory{std::make_unique<NBufferedMemory>(
          mem_map, psum_single_buf_size(), topo().NumOfPSumBuf,
          SLS_BLOCK_PSUM)},
      tags_memory{std::make_unique<NBufferedMemory>(
          mem_map, tags_single_buf_size(), topo().NumOfTagsBuf,
          SLS_BLOCK_TAGS)},
      min_psum_offsets_(topo().NumOfPSumBuf,
                        OffsetComputeUnitMap(topo().NumOfCUnits)),
      max_psum_offsets_(topo().NumOfPSumBuf,
                        OffsetComputeUnitMap(topo().NumOfCUnits)),
      min_tags_offsets_(topo().NumOfTagsBuf,
                        OffsetComputeUnitMap(topo().NumOfCUnits)),
      max_tags_offsets_(topo().NumOfTagsBuf,
                        OffsetComputeUnitMap(topo().NumOfCUnits)),
      mem_map_{mem_map} {
  // Fill `max_*_offsets_` with maximum possible offsets, so that the
  // buffers are later fully cleaned from possible garbage.
  const auto max_psum_offset = psum_single_buf_size();
  const auto max_tags_offset = tags_single_buf_size();
  for (auto &cunit_offset_map : max_psum_offsets_) {
    std::fill(cunit_offset_map.begin(), cunit_offset_map.end(),
              max_psum_offset);
  }
  for (auto &cunit_offset_map : max_tags_offsets_) {
    std::fill(cunit_offset_map.begin(), cunit_offset_map.end(),
              max_tags_offset);
  }

  // Fill `min_*_offsets_` with minimum possible offsets (zeros) for the same
  // purpose.
  for (auto &cunit_offset_map : min_psum_offsets_) {
    std::fill(cunit_offset_map.begin(), cunit_offset_map.end(), 0);
  }
  for (auto &cunit_offset_map : min_tags_offsets_) {
    std::fill(cunit_offset_map.begin(), cunit_offset_map.end(), 0);
  }
}
InterleavedPointer
AxdimmSimulatorCore::current_psum_buf(uint8_t compute_unit,
                                      uint32_t offset) const {
  return psum_memory->get_buffer(compute_unit, NBufferedMemory::Type::READ) +
         offset;
}

InterleavedPointer
AxdimmSimulatorCore::current_tags_buf(uint8_t compute_unit,
                                      uint32_t offset) const {
  return tags_memory->get_buffer(compute_unit, NBufferedMemory::Type::READ) +
         offset;
}

void AxdimmSimulatorCore::handle_psum_buf_read(uint8_t compute_unit,
                                               uint32_t offset) {
  const auto buf_idx =
      psum_memory->get_index(compute_unit, NBufferedMemory::Type::READ);
  clear_psum_buf(compute_unit, buf_idx, offset);
}

void AxdimmSimulatorCore::handle_tags_buf_read(uint8_t compute_unit,
                                               uint32_t offset) {
  const auto buf_idx =
      tags_memory->get_index(compute_unit, NBufferedMemory::Type::READ);
  clear_tags_buf(compute_unit, buf_idx, offset);
}

void AxdimmSimulatorCore::write_reg(uint8_t *reg_addr, uint32_t val) {
  *reinterpret_cast<uint32_t *>(reg_addr) = val;
}

template <typename T>
void AxdimmSimulatorCore::sparse_ft_sum(
    InterleavedPointer ft_table, InterleavedPointer psum,
    uint64_t bytes_per_idx) NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW {
  const size_t elements_per_idx = bytes_per_idx / sizeof(T);
  for (size_t sf = 0; sf < elements_per_idx; sf++) {
    *psum.as<T>() = (*psum.as<T>() + *ft_table.as<T>());
    psum += sizeof(T);
    ft_table += sizeof(T);
  }
}

void AxdimmSimulatorCore::sparse_ft_sum_tagged(
    InterleavedPointer ft_table, InterleavedPointer psum,
    InterleavedPointer tags,
    uint64_t bytes_per_idx) NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW {
  sparse_ft_sum<uint32_t>(ft_table, psum, bytes_per_idx);
  *tags.as<pnm::types::uint128_t>() +=
      *(ft_table + bytes_per_idx).as<pnm::types::uint128_t>();
}

void AxdimmSimulatorCore::exec_trace(uint8_t compute_unit) {
  const uint8_t *instr_buf = mem_map_[compute_unit].addr[SLS_BLOCK_INST];
  const uint8_t *cunit_base = mem_map_[compute_unit].addr[SLS_BLOCK_BASE];

  const auto HA = cunit_to_ha(compute_unit);
  const InterleavedPointer cfgr_base(
      mem_map_[compute_unit].addr[SLS_BLOCK_CFGR], HA);
  auto *processed_instr_num_reg =
      (cfgr_base + topo().RegPolling + POLL_INST_NUM_OFFSET).as<uint8_t>();

  const auto psum_buf_idx =
      psum_memory->get_index(compute_unit, NBufferedMemory::Type::WRITE);
  bool received_header = false;
  uint64_t bytes_per_idx = 0;

  bool is_tagged = false;
  const auto tags_buf_idx =
      tags_memory->get_index(compute_unit, NBufferedMemory::Type::WRITE);

  InterleavedPointer instr_rs(instr_buf, HA);

  const InterleavedPointer tables(cunit_base, HA);
  const auto psum =
      psum_memory->get_buffer(compute_unit, NBufferedMemory::Type::WRITE);

  const auto tags =
      tags_memory->get_buffer(compute_unit, NBufferedMemory::Type::WRITE);

  const uint32_t inst_per_single_buf_cached = inst_per_single_buf();
  for (uint32_t i = 0; i < inst_per_single_buf_cached; i++) {
    auto raw_instr = *instr_rs.as<uint64_t>();
    instr_rs += sizeof(uint64_t);

    if (!received_header) {
      const uint64_t mode = raw_instr & 0x03;
      bytes_per_idx = get_bytes_per_idx(mode);

      // Check the INSTR_HEADER_TAGBIT_POS bit. If 1 we have tagged sls.
      is_tagged = raw_instr & (1 << INSTR_HEADER_TAGBIT_POS);
      received_header = true;
    } else {
      const sls::RankedInstGenerator::DecodedInst instr =
          sls::RankedInstGenerator::decode_instruction(raw_instr);

      const uint64_t cunit_addr =
          sls::RankedInstGenerator::encode_address_from_instruction(instr);

      if (instr.opcode == sls::SPARSE_FT_SUM_F32) {
        sparse_ft_sum<float>(tables + cunit_addr,
                             psum + instr.output_idx * bytes_per_idx,
                             bytes_per_idx);
      } else if (instr.opcode == sls::SPARSE_FT_SUM_U32 ||
                 instr.opcode == sls::SPARSE_FT_SUM_U32_TAGGED) {
        if (!is_tagged) {
          sparse_ft_sum<uint32_t>(tables + cunit_addr,
                                  psum + instr.output_idx * bytes_per_idx,
                                  bytes_per_idx);
        } else {
          sparse_ft_sum_tagged(
              tables + cunit_addr, psum + instr.output_idx * bytes_per_idx,
              tags + instr.output_idx * topo().AlignedTagSize, bytes_per_idx);
        }
      } else {
        PNM_LOG_ERROR("Invalid opcode value: {:x}.", instr.opcode);
        std::abort();
      }

      // The instruction triggers a PSUM write to the range [start, end),
      // which is defined as follows:
      const uint64_t psum_write_size = bytes_per_idx;
      const uint64_t psum_write_start_offset =
          instr.output_idx * psum_write_size;
      const uint64_t psum_write_end_offset =
          psum_write_start_offset + psum_write_size;

      // Update PSUM dirty range boundaries.
      min_psum_offsets_[psum_buf_idx][compute_unit] =
          std::min(min_psum_offsets_[psum_buf_idx][compute_unit],
                   psum_write_start_offset);
      max_psum_offsets_[psum_buf_idx][compute_unit] = std::max(
          max_psum_offsets_[psum_buf_idx][compute_unit], psum_write_end_offset);

      if (is_tagged) {
        // Do the same with the TAGS buffer.
        const uint64_t tags_write_size = topo().AlignedTagSize;
        const uint64_t tags_write_start_offset =
            instr.output_idx * tags_write_size;
        const uint64_t tags_write_end_offset =
            tags_write_start_offset + tags_write_size;

        min_tags_offsets_[tags_buf_idx][compute_unit] =
            std::min(min_tags_offsets_[tags_buf_idx][compute_unit],
                     tags_write_start_offset);
        max_tags_offsets_[tags_buf_idx][compute_unit] =
            std::max(max_tags_offsets_[tags_buf_idx][compute_unit],
                     tags_write_end_offset);
      }

      if (instr.trace_end) {
        /* set number of processed instructions register (including last
         * instruction, with enabled 'trace_end' bit) */
        write_reg(processed_instr_num_reg, i + 1);
        break;
      }
      assert(i < (inst_per_single_buf() - 1) || instr.trace_end);
    }
  }
}

void AxdimmSimulatorCore::shift_buffers(uint8_t compute_unit) {
  psum_memory->shift_buffers(compute_unit);
  tags_memory->shift_buffers(compute_unit);
}

void AxdimmSimulatorCore::clear_memory(InterleavedPointer ptr, size_t size) {
  assert(size % topo().RankInterleavingSize == 0);

  for (size_t i = 0; i < size; i += topo().RankInterleavingSize) {
    memset(ptr.get(), 0, topo().RankInterleavingSize);
    ptr += topo().RankInterleavingSize;
  }
}

void AxdimmSimulatorCore::clear_buf_common(OffsetBufMapRef min_offsets,
                                           OffsetBufMapRef max_offsets,
                                           NBufferedMemory &memory,
                                           uint64_t buffer_size,
                                           uint8_t compute_unit,
                                           uint8_t buf_idx, uint32_t offset) {
  assert(compute_unit < topo().NumOfCUnits);
  assert(buf_idx < topo().NumOfPSumBuf);

  // Zero out only the part of the buffer that has been modified.
  // The size of the range to be cleared must be divisible by
  // RANK_INTERLEAVING_SIZE, so we align both boundary points by this value.
  const auto max_offset = max_offsets[buf_idx][compute_unit];
  const auto min_offset = min_offsets[buf_idx][compute_unit];
  if (min_offset >= max_offset) {
    // Nothing to clear.
    return;
  }

  const auto clear_start =
      align_down<uint32_t>(min_offset, topo().RankInterleavingSize);
  const auto clear_end =
      align_up<uint32_t>(max_offset, topo().RankInterleavingSize);
  const auto clear_size = clear_end - clear_start;

  assert(clear_size % topo().RankInterleavingSize == 0);
  assert(clear_end <= buffer_size);
  clear_memory(memory.get_buffer_by_index(compute_unit, buf_idx) + offset,
               clear_size);

  // Re-initialize offset statistics with neutral values w.r.t. the `max` and
  // `min` operations.
  max_offsets[buf_idx][compute_unit] = 0;
  min_offsets[buf_idx][compute_unit] = buffer_size;
}

// Clear a single PSUM buffer.
void AxdimmSimulatorCore::clear_psum_buf(uint8_t compute_unit, uint8_t buf_idx,
                                         uint32_t offset) {
  clear_buf_common(min_psum_offsets_.data(), max_psum_offsets_.data(),
                   *psum_memory, psum_single_buf_size(), compute_unit, buf_idx,
                   offset);
}

// Clear a single TAGS buffer.
void AxdimmSimulatorCore::clear_tags_buf(uint8_t compute_unit, uint8_t buf_idx,
                                         uint32_t offset) {
  clear_buf_common(min_tags_offsets_.data(), max_tags_offsets_.data(),
                   *tags_memory, tags_single_buf_size(), compute_unit, buf_idx,
                   offset);
}

void AxdimmSimulatorCore::clear_buffers(uint8_t compute_unit) {
  for (uint8_t buf_idx = 0; buf_idx < topo().NumOfPSumBuf; ++buf_idx) {
    clear_psum_buf(compute_unit, buf_idx);
  }
  for (uint8_t buf_idx = 0; buf_idx < topo().NumOfTagsBuf; ++buf_idx) {
    clear_tags_buf(compute_unit, buf_idx);
  }
}

} // namespace pnm::sls::device
