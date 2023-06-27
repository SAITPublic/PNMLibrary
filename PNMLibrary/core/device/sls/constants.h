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

#include <cstddef>
#include <cstdint>

namespace pnm::sls::device {

// Derived topology constants.

inline size_t psum_single_buf_size() {
  using pnm::device::topo;
  return topo().PSumBufSize / topo().NumOfPSumBuf;
}

inline size_t tags_single_buf_size() {
  using pnm::device::topo;
  return topo().TagsBufSize / topo().NumOfTagsBuf;
}

inline size_t inst_single_buf_size() {
  using pnm::device::topo;
  return topo().InstBufSize / topo().NumOfInstBuf;
}

inline size_t inst_per_single_buf() {
  using pnm::device::topo;
  return inst_single_buf_size() / topo().InstructionSize;
}

inline size_t trace_header_size() {
  return pnm::device::topo().InstructionSize;
}

inline size_t inst_per_single_buf_no_header() {
  using pnm::device::topo;
  return (inst_single_buf_size() - trace_header_size()) /
         topo().InstructionSize;
}

// Misc SLS constants.
// [TODO: MCS23-1393] find better place for this constants
inline constexpr uint32_t POLL_FINISH_VALUE = 0xFFFFFFFF;
inline constexpr uint32_t POLL_INST_NUM_OFFSET = 8;

} // namespace pnm::sls::device

#endif // _SLS_CONSTANTS_H_
