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
#ifndef _AXDIMM_HW_CONFIG_H_
#define _AXDIMM_HW_CONFIG_H_

// [TODO: AxdimmSystemDevice] cleanup

/* Number of elements for dummy instruction */
#define DUMMY_INST_SIZE 16

#define COL0_RA_OFFSET (3)
#define COL0_MASK (0x07)
#define COL0_START_BIT (0)
#define COL1_RA_OFFSET (7)
#define COL1_MASK (0x07f)
#define COL1_START_BIT (3)
#define ROW0_RA_OFFSET (14)
#define ROW0_MASK (0x07)
#define ROW0_START_BIT (0)
#define ROW1_RA_OFFSET (20)
#define ROW1_MASK (0x01)
#define ROW1_START_BIT (3)
#define ROW2_RA_OFFSET (28)
#define ROW2_MASK (0x01)
#define ROW2_START_BIT (4)
#define ROW3_RA_OFFSET (21)
#define ROW3_MASK (0x07F)
#define ROW3_START_BIT (5)
#define ROW4_RA_OFFSET (29)
#define ROW4_MASK (0x01F)
#define ROW4_START_BIT (12)
#define BG0_ARG0_RA_OFFSET (20)
#define BG0_ARG1_RA_OFFSET (6)
#define BG1_ARG0_RA_OFFSET (21)
#define BG1_ARG1_RA_OFFSET (17)
#define BANK0_ARG0_RA_OFFSET (22)
#define BANK0_ARG1_RA_OFFSET (18)
#define BANK1_ARG0_RA_OFFSET (23)
#define BANK1_ARG1_RA_OFFSET (19)
#define CS_OFFSET (34)
#define CS_MASK (0x1)

/* SLS instruction layout */
#define INSTR_OPCODE_OFFSET (62)
#define INSTR_OPCODE_MASK (0x03ULL)
#define INSTR_LOCAL_BIT_OFFSET (61)
#define INSTR_LOCAL_BIT_MASK (0x01ULL)
#define INSTR_OUTPUT_IDX_OFFSET (49)
#define INSTR_OUTPUT_IDX_MASK (0x0FFFULL)
#define INSTR_TRACE_END_OFFSET (48)
#define INSTR_TRACE_END_MASK (0x01ULL)
#define INSTR_ROW_OFFSET (14)
#define INSTR_ROW_MASK (0x01FFFFULL)
#define INSTR_BANKGROUP_OFFSET (12)
#define INSTR_BANKGROUP_MASK (0x03ULL)
#define INSTR_BANK_OFFSET (10)
#define INSTR_BANK_MASK (0x03ULL)
#define INSTR_COL_OFFSET (0)
#define INSTR_COL_MASK (0x03FFULL)
#define INSTR_HEADER_TAGBIT_POS (8)
#define INSTR_HEADER_VSIZE_MASK (0b11111ULL)
#define INSTR_CS_OFFSET (31)
#define INSTR_CS_MASK (0x1)

#endif
