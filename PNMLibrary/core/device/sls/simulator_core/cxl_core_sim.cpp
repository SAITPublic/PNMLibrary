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

#include "cxl_core_sim.h"

#include "core/device/sls/utils/constants.h"
#include "core/device/sls/utils/inst_generator.h"
#include "core/device/sls/utils/memory_map.h"

#include "common/compiler_internal.h"
#include "common/make_error.h"
#include "common/topology_constants.h"
#include "hw/pnm_hw_instr_constants.h"

#include "pnmlib/common/128bit_math.h"
#include "pnmlib/common/views.h"

#include <linux/sls_resources.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>

namespace {

inline uint64_t get_bytes_per_idx(uint64_t mode) {
  assert(mode <= 2);
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

CxlSimulatorCore::CxlSimulatorCore(const MemMap &mem_map) : mem_map_{mem_map} {}

void CxlSimulatorCore::write_reg(uint8_t *reg_addr, uint32_t val) {
  *reinterpret_cast<volatile uint32_t *>(reg_addr) = val;
}

template <typename T>
void CxlSimulatorCore::sparse_ft_sum(const uint8_t *channel_base,
                                     uint64_t channel_addr, uint32_t output_idx,
                                     uint8_t *psum_buf,
                                     uint64_t bytes_per_idx) {
  const auto *const table =
      reinterpret_cast<const T *>(channel_base + channel_addr);
  auto psum = std::launder(
      reinterpret_cast<T *>(psum_buf + output_idx * bytes_per_idx));

  const size_t elements_per_idx = bytes_per_idx / sizeof(T);

  std::transform(table, table + elements_per_idx, psum, psum, std::plus{});
}

void CxlSimulatorCore::sparse_ft_sum_tagged(
    const uint8_t *channel_base, uint64_t channel_addr, uint32_t output_idx,
    uint8_t *psum_buf, uint8_t *tags_buf,
    uint64_t bytes_per_idx) NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW {
  sparse_ft_sum<uint32_t>(channel_base, channel_addr, output_idx, psum_buf,
                          bytes_per_idx);
  const auto *const tag_table = reinterpret_cast<const pnm::types::uint128_t *>(
      channel_base + channel_addr + bytes_per_idx);

  auto *const tag_psum = reinterpret_cast<pnm::types::uint128_t *>(
      tags_buf + output_idx * topo().AlignedTagSize);

  *tag_psum += *tag_table;
}

void CxlSimulatorCore::exec_trace(uint8_t channel) {
  const uint8_t *instr_buf = mem_map_[channel].addr[SLS_BLOCK_INST];
  const uint8_t *channel_base = mem_map_[channel].addr[SLS_BLOCK_BASE];
  uint8_t *processed_instr_num_reg = mem_map_[channel].addr[SLS_BLOCK_CFGR] +
                                     topo().RegPolling + POLL_INST_NUM_OFFSET;
  uint8_t *psum_buf = mem_map_[channel].addr[SLS_BLOCK_PSUM];
  bool received_header = false;
  uint64_t bytes_per_idx = 0;

  bool is_tagged = false;
  uint8_t *tags_buf = mem_map_[channel].addr[SLS_BLOCK_TAGS];

  const uint32_t inst_per_single_buf_cached = inst_per_single_buf();
  const auto *const instr_begin = reinterpret_cast<const uint64_t *>(instr_buf);
  const auto instructions = pnm::views::make_view(
      instr_begin, instr_begin + inst_per_single_buf_cached);

  for (uint32_t i = 0; i < inst_per_single_buf_cached; i++) {
    const uint64_t raw_instr = instructions[i];
    if (!received_header) {
      const uint64_t mode = raw_instr & 0x03;
      bytes_per_idx = get_bytes_per_idx(mode);

      // Check the INSTR_HEADER_TAGBIT_POS bit. If 1 we have tagged sls.
      is_tagged = raw_instr & (1 << INSTR_HEADER_TAGBIT_POS);
      received_header = true;
    } else {
      const auto instr =
          sls::ChannelInstGenerator::decode_instruction(raw_instr);
      const uint64_t channel_addr =
          sls::ChannelInstGenerator::encode_address_from_instruction(instr);

      if (instr.opcode == sls::SPARSE_FT_SUM_F32) {
        sparse_ft_sum<float>(channel_base, channel_addr, instr.output_idx,
                             psum_buf, bytes_per_idx);
      } else if (instr.opcode == sls::SPARSE_FT_SUM_U32 ||
                 instr.opcode == sls::SPARSE_FT_SUM_U32_TAGGED) {
        if (!is_tagged) {
          sparse_ft_sum<uint32_t>(channel_base, channel_addr, instr.output_idx,
                                  psum_buf, bytes_per_idx);
        } else {
          sparse_ft_sum_tagged(channel_base, channel_addr, instr.output_idx,
                               psum_buf, tags_buf, bytes_per_idx);
        }
      } else {
        throw pnm::error::make_inval("Opcode value: {:x}.", instr.opcode);
      }

      if (instr.trace_end) {
        /* set number of processed instructions register (including last
         * instruction, with enabled 'trace_end' bit) */
        write_reg(processed_instr_num_reg, i + 1);
        break;
      }
      assert(i < (inst_per_single_buf_cached - 1) || instr.trace_end);
    }
  }
}

void CxlSimulatorCore::clear_buffers([[maybe_unused]] uint8_t channel) {
  // Not applicable for CXL-SLS because we don't have clean operation in device.
  // [TODO: @y-lavrinenko] Ger rid after simulator refactor
}

} // namespace pnm::sls::device
