/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics. No part of this
 * software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 */

#ifndef PNM_ATOMIC_H
#define PNM_ATOMIC_H

#include "barrier.h"

#include "common/compiler_internal.h"

#include <cstdint>

namespace pnm::memory {

ALWAYS_INLINE void hw_atomic_store(
    volatile uint32_t *reg, // NOLINT(readability-non-const-parameter)
    uint32_t value) {
  // Here we implement atomic store operation to set value in
  // HW registers. We use own implementation because several
  // compilers implement atomic operation by `xchg` operand
  // that fail to execute on the current PoC HW (leads to system
  // reboots and hangs). To avoid this, we explicitly use
  // `mov` operand followed by memory fence.

  if constexpr (TSAN) {
    // Suppress TSAN error, because without std::atomic tsan is unable to detect
    // instruction relations
    __tsan_release(const_cast<uint32_t *>(reg));
  }

  asm inline volatile(
      "mov {%[src_value], %[dst_mem] | %[dst_mem], %[src_value]}\n\t"
      : [dst_mem] "=m"(*reg)
      : [src_value] "r"(value));

  mfence(); // No load operation before mfence, all stores are done
}

} // namespace pnm::memory

#endif // PNM_ATOMIC_H
