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

#ifndef _SLS_CONSTANTS_H_
#define _SLS_CONSTANTS_H_

#include <common/topology_constants.h>

#include <bitset>
#include <cstddef>
#include <cstdint>

namespace pnm::sls::device {

// Derived topology constants.

inline size_t psum_single_buf_size() {
  return topo().PSumBufSize / topo().NumOfPSumBuf;
}

inline size_t tags_single_buf_size() {
  return topo().TagsBufSize / topo().NumOfTagsBuf;
}

inline size_t inst_single_buf_size() {
  return topo().InstBufSize / topo().NumOfInstBuf;
}

inline size_t inst_per_single_buf() {
  return inst_single_buf_size() / topo().InstructionSize;
}

inline size_t trace_header_size() { return topo().InstructionSize; }

inline size_t inst_per_single_buf_no_header() {
  return (inst_single_buf_size() - trace_header_size()) /
         topo().InstructionSize;
}

// SLS mask
inline constexpr auto cunit_mask_size = 64ULL;
using cunit = std::bitset<cunit_mask_size>;

// Misc SLS constants.
// [TODO: MCS23-1393] find better place for this constants
inline constexpr uint32_t POLL_FINISH_VALUE = 0xFFFFFFFF;
inline constexpr uint32_t POLL_INST_NUM_OFFSET = 8;

// Set of predefined values to control SLS device
// For detail see confluence page:
// https://confluence.samsungds.net/display/NP/SLS+operation+on+CXL+device
inline constexpr uint32_t SLS_ENABLE_VALUE = 0xCAFE;
inline constexpr uint32_t SLS_DISABLE_VALUE = 0;
inline constexpr uint32_t SLS_CONTROL_SWITCHED_VALUE = 0x10001;

} // namespace pnm::sls::device

#endif // _SLS_CONSTANTS_H_
