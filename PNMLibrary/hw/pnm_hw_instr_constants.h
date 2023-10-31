/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 */
#ifndef _PNM_HW_CONSTANTS_H_
#define _PNM_HW_CONSTANTS_H_

#include <cstdint>

namespace pnm::hw {
/* Number of elements for dummy instruction */
inline constexpr uint64_t DUMMY_INST_SIZE = 16;

/* SLS instruction layout */
inline constexpr uint64_t INSTR_OPCODE_OFFSET = 62;
inline constexpr uint64_t INSTR_OPCODE_MASK = 0x03;
inline constexpr uint64_t INSTR_LOCAL_BIT_OFFSET = 61;
inline constexpr uint64_t INSTR_LOCAL_BIT_MASK = 0x01;
inline constexpr uint64_t INSTR_OUTPUT_IDX_OFFSET = 49;
inline constexpr uint64_t INSTR_OUTPUT_IDX_MASK = 0x0FFF;
inline constexpr uint64_t INSTR_TRACE_END_OFFSET = 48;
inline constexpr uint64_t INSTR_TRACE_END_MASK = 0x01;
inline constexpr uint64_t INSTR_ROW_OFFSET = 14;
inline constexpr uint64_t INSTR_ROW_MASK = 0x01FFFF;
inline constexpr uint64_t INSTR_BANKGROUP_OFFSET = 12;
inline constexpr uint64_t INSTR_BANKGROUP_MASK = 0x03;
inline constexpr uint64_t INSTR_BANK_OFFSET = 10;
inline constexpr uint64_t INSTR_BANK_MASK = 0x03;
inline constexpr uint64_t INSTR_COL_OFFSET = 0;
inline constexpr uint64_t INSTR_COL_MASK = 0x03FF;
inline constexpr uint64_t INSTR_HEADER_TAGBIT_POS = 8;
inline constexpr uint64_t INSTR_HEADER_VSIZE_MASK = 0b11111;
} // namespace pnm::hw

#endif
