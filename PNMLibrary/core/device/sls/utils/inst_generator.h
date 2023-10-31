/*
 * Copyright (C) 2021 Samsung Electronics Co. LTD
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

#ifndef _SLS_INST_GENERATOR_H_
#define _SLS_INST_GENERATOR_H_

#include "hw/pnm_addr_constants.h"
#include "hw/pnm_hw_instr_constants.h"

#include "pnmlib/common/definitions.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace pnm::hw;

namespace pnm::sls {

enum Opcode : uint8_t {
  SPARSE_FT_SUM_F32,
  RESERVED, // Reserved for 'Fully Connected' operation Opcode
  SPARSE_FT_SUM_U32,
  SPARSE_FT_SUM_U32_TAGGED,
};

inline const std::vector<uint64_t> dummy_inst = [] {
  std::vector<uint64_t> vec(pnm::hw::DUMMY_INST_SIZE, 0xffd000000000000);
  vec[0] = 0;
  return vec;
}();

class RankedInstGenerator {
public:
  struct DecodedInst {
    uint32_t opcode = 0;
    uint32_t local_bit = 0;
    uint32_t output_idx = 0;
    uint32_t trace_end = 0;
    uint32_t row = 0;
    uint32_t bankgroup = 0;
    uint32_t bank = 0;
    uint32_t column = 0;
  };

  static DecodedInst make_empty_instruction(uint32_t opcode,
                                            uint32_t output_idx) {
    return DecodedInst{opcode, false, output_idx, false};
  }

  static uint64_t encode_instruction(const DecodedInst &inst) {
    if constexpr (DEBUG) {
      [[maybe_unused]] const int bit_width[8] = {2, 1, 12, 1, 17, 2, 2, 10};
      for (size_t i = 0; i < sizeof(DecodedInst) / sizeof(uint32_t); i++) {
        assert(((uint32_t *)&inst)[i] < ((uint32_t)1 << bit_width[i]));
      }
    }

    assert(inst.opcode == SPARSE_FT_SUM_F32 || inst.opcode == RESERVED ||
           inst.opcode == SPARSE_FT_SUM_U32 ||
           inst.opcode == SPARSE_FT_SUM_U32_TAGGED);

    const uint64_t tmp =
        ((uint64_t)inst.opcode << INSTR_OPCODE_OFFSET) +
        ((uint64_t)inst.local_bit << INSTR_LOCAL_BIT_OFFSET) +
        ((uint64_t)inst.output_idx << INSTR_OUTPUT_IDX_OFFSET) +
        ((uint64_t)inst.trace_end << INSTR_TRACE_END_OFFSET) +
        ((uint64_t)inst.row << INSTR_ROW_OFFSET) +
        ((uint64_t)inst.bankgroup << INSTR_BANKGROUP_OFFSET) +
        ((uint64_t)inst.bank << INSTR_BANK_OFFSET) +
        ((uint64_t)inst.column << INSTR_COL_OFFSET);

    return tmp;
  }

  static DecodedInst decode_instruction(uint64_t input_instr) {
    DecodedInst instr = {
        .opcode = (uint32_t)((input_instr >> INSTR_OPCODE_OFFSET) &
                             INSTR_OPCODE_MASK),
        .local_bit = (uint32_t)((input_instr >> INSTR_LOCAL_BIT_OFFSET) &
                                INSTR_LOCAL_BIT_MASK),
        .output_idx = (uint32_t)((input_instr >> INSTR_OUTPUT_IDX_OFFSET) &
                                 INSTR_OUTPUT_IDX_MASK),
        .trace_end = (uint32_t)((input_instr >> INSTR_TRACE_END_OFFSET) &
                                INSTR_TRACE_END_MASK),
        .row = (uint32_t)((input_instr >> INSTR_ROW_OFFSET) & INSTR_ROW_MASK),
        .bankgroup = (uint32_t)((input_instr >> INSTR_BANKGROUP_OFFSET) &
                                INSTR_BANKGROUP_MASK),
        .bank =
            (uint32_t)((input_instr >> INSTR_BANK_OFFSET) & INSTR_BANK_MASK),
        .column =
            (uint32_t)((input_instr >> INSTR_COL_OFFSET) & INSTR_COL_MASK),
    };

    return instr;
  }

  static uint64_t encode_address_from_instruction(const DecodedInst &inst) {

    uint64_t rank_addr = 0;

    // COLUMN
    for (const auto &entry : COL) {
      rank_addr += ((inst.column >> entry.StartBit) & entry.Mask)
                   << entry.RAOffset;
    }

    // ROW
    for (const auto &entry : ROW) {
      rank_addr += ((inst.row >> entry.StartBit) & entry.Mask)
                   << entry.RAOffset;
    }

    // BankGroup
    for (const auto &entry : BG) {
      rank_addr += (((inst.bankgroup >> entry.StartBit) & 0b1) ^
                    ((rank_addr >> entry.RAOffset0) & 0b1))
                   << entry.RAOffset1;
    }

    // Bank
    for (const auto &entry : BANK) {
      rank_addr += (((inst.bank >> entry.StartBit) & 0b1) ^
                    ((rank_addr >> entry.RAOffset0) & 0b1))
                   << entry.RAOffset1;
    }

    return rank_addr;
  }

  static void decode_address_to_instruction(uint64_t rank_addr,
                                            DecodedInst &inst) {
    assert(rank_addr % 4 == 0); // 4 - Byte

    inst.bankgroup = 0;
    for (const auto &entry : BG) {
      inst.bankgroup += (((rank_addr >> entry.RAOffset0) & 0b1) ^
                         ((rank_addr >> entry.RAOffset1) & 0b1))
                        << entry.StartBit;
    }

    inst.bank = 0;
    for (const auto &entry : BANK) {
      inst.bank += (((rank_addr >> entry.RAOffset0) & 0b1) ^
                    ((rank_addr >> entry.RAOffset1) & 0b1))
                   << entry.StartBit;
    }

    // Row
    inst.row = 0;
    for (const auto &entry : ROW) {
      inst.row += ((rank_addr >> entry.RAOffset) & entry.Mask)
                  << entry.StartBit;
    }

    // Column
    inst.column = 0;
    for (const auto &entry : COL) {
      inst.column += ((rank_addr >> entry.RAOffset) & entry.Mask)
                     << entry.StartBit;
    }
  }
};

class ChannelInstGenerator {
public:
  using DecodedInst = RankedInstGenerator::DecodedInst;

  static DecodedInst make_empty_instruction(uint32_t opcode,
                                            uint32_t output_idx) {
    return RankedInstGenerator::make_empty_instruction(opcode, output_idx);
  }

  static uint64_t encode_instruction(const DecodedInst &inst) {
    return RankedInstGenerator::encode_instruction(inst);
  }

  static DecodedInst decode_instruction(uint64_t input_instr) {
    return RankedInstGenerator::decode_instruction(input_instr);
  }

  static uint64_t encode_address_from_instruction(const DecodedInst &inst) {
    // The channel address is a rank address with modification in CS, BA and BG
    // bits.
    uint64_t channel_addr =
        RankedInstGenerator::encode_address_from_instruction(inst);

    // Rewrite bank group and bank address bits for CXL-SLS channel address
    for (const auto &entry : BANK) {
      channel_addr &= ~(0b1ULL << entry.RAOffset1);
      channel_addr |= ((inst.bank >> entry.StartBit) & 0b1) << entry.RAOffset1;
    }

    for (const auto &entry : BG) {
      channel_addr &= ~(0b1ULL << entry.RAOffset1);
      channel_addr |= ((inst.bankgroup >> entry.StartBit) & 0b1)
                      << entry.RAOffset1;
    }

    return channel_addr;
  }

  static void decode_address_to_instruction(uint64_t channel_addr,
                                            DecodedInst &inst) {
    RankedInstGenerator::decode_address_to_instruction(channel_addr, inst);

    // We found that address for CXL-SLS is different from AXDIMM. The
    // difference is not only in CS but also in bankgroup and bank.
    // The evaluation of bankgroup and bank addresses looks wrong, but it allows
    // to work with device in range up to (1 << 20)B with good matching. The
    // larger addresses should be fine to, but we can't check it because device
    // hangs. Also, if device firmware updated we should check the instruction
    // generation.
    inst.bankgroup = 0;
    for (const auto &entry : BG) {
      inst.bankgroup |= ((channel_addr >> entry.RAOffset1) & 0b1)
                        << entry.StartBit;
    }

    inst.bank = 0;
    for (const auto &entry : BANK) {
      inst.bank |= ((channel_addr >> entry.RAOffset1) & 0b1) << entry.StartBit;
    }
  }
};
} // namespace pnm::sls

#endif
