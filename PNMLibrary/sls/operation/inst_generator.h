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

#include "hw/axdimm_hw_config.h"

#include "pnmlib/common/definitions.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace pnm::sls {

enum Opcode : uint32_t {
  SPARSE_FT_SUM_F32,
  RESERVED, // Reserved for 'Fully Connected' operation Opcode
  SPARSE_FT_SUM_U32,
  SPARSE_FT_SUM_U32_TAGGED,
};

inline const std::vector<uint64_t> dummy_inst = [] {
  std::vector<uint64_t> vec(DUMMY_INST_SIZE, 0xffd000000000000);
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

    // column: [ [3:5] [7:13] ]
    rank_addr +=
        (((inst.column >> COL0_START_BIT) & COL0_MASK) << COL0_RA_OFFSET) +
        (((inst.column >> COL1_START_BIT) & COL1_MASK) << COL1_RA_OFFSET);
    // row: [ [14:16] [20] [28] [21:27] [29:33] ]
    rank_addr +=
        (((inst.row >> ROW0_START_BIT) & ROW0_MASK) << ROW0_RA_OFFSET) +
        (((inst.row >> ROW1_START_BIT) & ROW1_MASK) << ROW1_RA_OFFSET) +
        (((inst.row >> ROW2_START_BIT) & ROW2_MASK) << ROW2_RA_OFFSET) +
        (((inst.row >> ROW3_START_BIT) & ROW3_MASK) << ROW3_RA_OFFSET) +
        /* casting required since row slice #4 operates on bits[29:33] */
        ((((uint64_t)inst.row >> ROW4_START_BIT) & ROW4_MASK)
         << ROW4_RA_OFFSET);
    // bit_6 = bg[0] ^ bit_20
    rank_addr +=
        ((inst.bankgroup & 0x01) ^ ((rank_addr >> BG0_ARG0_RA_OFFSET) & 0x01))
        << BG0_ARG1_RA_OFFSET;
    // bit_17 = bg[1] ^ bit_21
    rank_addr += (((inst.bankgroup >> 1) & 0x01) ^
                  ((rank_addr >> BG1_ARG0_RA_OFFSET) & 0x01))
                 << BG1_ARG1_RA_OFFSET;
    // bit_18 = bank[0] ^ bit_22
    rank_addr +=
        ((inst.bank & 0x01) ^ ((rank_addr >> BANK0_ARG0_RA_OFFSET) & 0x01))
        << BANK0_ARG1_RA_OFFSET;
    // bit_19 = bank[1] ^ bit_23
    rank_addr += (((inst.bank >> 1) & 0x01) ^
                  ((rank_addr >> BANK1_ARG0_RA_OFFSET) & 0x01))
                 << BANK1_ARG1_RA_OFFSET;

    return rank_addr;
  }

  static void decode_address_to_instruction(uint64_t rank_addr,
                                            DecodedInst &inst) {
    assert(rank_addr % 4 == 0); // 4 - Byte
    // Bank Group_0
    inst.bankgroup = ((rank_addr >> BG0_ARG0_RA_OFFSET) & 0x1) ^
                     ((rank_addr >> BG0_ARG1_RA_OFFSET) & 0x1);
    // Bank Group_1
    inst.bankgroup += (((rank_addr >> BG1_ARG0_RA_OFFSET) & 0x1) ^
                       ((rank_addr >> BG1_ARG1_RA_OFFSET) & 0x1))
                      << 1;
    // Bank_0
    inst.bank = ((rank_addr >> BANK0_ARG0_RA_OFFSET) & 0x1) ^
                ((rank_addr >> BANK0_ARG1_RA_OFFSET) & 0x1);
    // Bank_1
    inst.bank += (((rank_addr >> BANK1_ARG0_RA_OFFSET) & 0x1) ^
                  ((rank_addr >> BANK1_ARG1_RA_OFFSET) & 0x1))
                 << 1;
    // Row
    inst.row = (((rank_addr >> ROW4_RA_OFFSET) & ROW4_MASK) << ROW4_START_BIT) +
               (((rank_addr >> ROW3_RA_OFFSET) & ROW3_MASK) << ROW3_START_BIT) +
               (((rank_addr >> ROW2_RA_OFFSET) & ROW2_MASK) << ROW2_START_BIT) +
               (((rank_addr >> ROW1_RA_OFFSET) & ROW1_MASK) << ROW1_START_BIT) +
               (((rank_addr >> ROW0_RA_OFFSET) & ROW0_MASK) << ROW0_START_BIT);
    // Column
    inst.column =
        (((rank_addr >> COL0_RA_OFFSET) & COL0_MASK) << COL0_START_BIT) +
        (((rank_addr >> COL1_RA_OFFSET) & COL1_MASK) << COL1_START_BIT);
  }
};

class ChannelInstGenerator {
public:
  struct DecodedInst {
    uint32_t cs = 0;
    RankedInstGenerator::DecodedInst ranked_instruction{};
  };

  static DecodedInst make_empty_instruction(uint32_t opcode,
                                            uint32_t output_idx) {
    return DecodedInst{
        .cs = 0,
        .ranked_instruction =
            RankedInstGenerator::make_empty_instruction(opcode, output_idx),
    };
  }

  static uint64_t encode_instruction(const DecodedInst &inst) {
    auto tmp = RankedInstGenerator::encode_instruction(inst.ranked_instruction);
    if constexpr (DEBUG) {
      assert(inst.cs <= 1 && "CS should be 0 or 1");
    }

    tmp |= static_cast<uint64_t>(inst.cs) << INSTR_CS_OFFSET;

    return tmp;
  }

  static DecodedInst decode_instruction(uint64_t input_instr) {
    DecodedInst instr{.cs = static_cast<uint32_t>(
                          (input_instr >> INSTR_CS_OFFSET) & INSTR_CS_MASK),
                      .ranked_instruction =
                          RankedInstGenerator::decode_instruction(input_instr)};

    return instr;
  }

  static uint64_t encode_address_from_instruction(const DecodedInst &inst) {
    // The channel address is a rank address with modification in CS, BA and BG
    // bits.
    uint64_t channel_addr =
        RankedInstGenerator::encode_address_from_instruction(
            inst.ranked_instruction);

    // Rewrite bank group and bank address bits for CXL-SLS channel address
    static constexpr auto BA_MASK = 0b11ULL << BANK0_ARG1_RA_OFFSET;
    channel_addr &= ~BA_MASK;
    channel_addr |= inst.ranked_instruction.bank << BANK0_ARG1_RA_OFFSET;

    channel_addr &= ~(0b1ULL << BG0_ARG1_RA_OFFSET);
    channel_addr |= (inst.ranked_instruction.bankgroup & 0b1)
                    << BG0_ARG1_RA_OFFSET;

    channel_addr &= ~(0b1ULL << BG1_ARG1_RA_OFFSET);
    channel_addr |= ((inst.ranked_instruction.bankgroup >> 1) & 0b1)
                    << BG1_ARG1_RA_OFFSET;

    // Store CS
    channel_addr |= (static_cast<uint64_t>(inst.cs & CS_MASK) << CS_OFFSET);
    return channel_addr;
  }

  static void decode_address_to_instruction(uint64_t channel_addr,
                                            DecodedInst &inst) {
    RankedInstGenerator::decode_address_to_instruction(channel_addr,
                                                       inst.ranked_instruction);

    // We found that address for CXL-SLS is different from AXDIMM. The
    // difference is not only in CS but also in bankgroup and bank.
    // The evaluation of bankgroup and bank addresses looks wrong, but it allows
    // to work with device in range up to (1 << 20)B with good matching. The
    // larger addresses should be fine to, but we can't check it because device
    // hangs. Also, if device firmware updated we should check the instruction
    // generation.
    inst.ranked_instruction.bankgroup =
        (channel_addr >> BG0_ARG1_RA_OFFSET) & 0x1;
    inst.ranked_instruction.bankgroup |=
        ((channel_addr >> BG1_ARG1_RA_OFFSET) & 0x1) << 1;

    inst.ranked_instruction.bank = (channel_addr >> BANK0_ARG1_RA_OFFSET) & 0x3;
    // CS
    inst.cs = (channel_addr >> CS_OFFSET) & CS_MASK;
  }
};
} // namespace pnm::sls

#endif
