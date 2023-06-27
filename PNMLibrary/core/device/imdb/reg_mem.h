//=- core/device/imdb/reg_mem.h - IMDB registers presentation -*- C++-*-----=//

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

#ifndef _IMDB_REG_MEM_H_
#define _IMDB_REG_MEM_H_

#include "pnmlib/imdb/libimdb.h"

#include "pnmlib/common/views.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace pnm::imdb::device {

inline auto regs_as_ints(ThreadCSR *regs) {
  return pnm::view_cast<uint32_t>(pnm::make_view(regs, 1));
}

inline auto regs_as_ints(const ThreadCSR *regs) {
  return pnm::view_cast<const uint32_t>(pnm::make_view(regs, 1));
}

template <typename T, std::size_t size>
auto to_volatile_ptrs(std::array<volatile T, size> &values_arr) {
  std::array<volatile T *, size> ptrs_arr;

  for (std::size_t i = 0; i < size; ++i) {
    ptrs_arr[i] = &values_arr[i];
  }

  return ptrs_arr;
};

/** @brief Collection of pointers to all IMDB registers.
 */
struct RegisterPointers {
  /** @brief Pointer to the global device registers (common). */
  volatile CommonCSR *common;
  /** @brief Pointer to the global device registers (status). */
  volatile StatusCSR *status;
  /** @brief Pointers to per-thread device registers. */
  std::array<volatile ThreadCSR *, MAX_THREADS> thread;
};

} // namespace pnm::imdb::device

#endif /* _IMDB_REG_MEM_H_ */
