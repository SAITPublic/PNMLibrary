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

#ifndef _USER_BARRIER_H_
#define _USER_BARRIER_H_

#include "common/compiler_internal.h"

#include <cstdint>

namespace pnm::memory {

ALWAYS_INLINE void mfence() { asm volatile("mfence" ::: "memory"); }

ALWAYS_INLINE void lfence() { asm volatile("lfence" ::: "memory"); }

ALWAYS_INLINE void sfence() { asm volatile("sfence" ::: "memory"); }

ALWAYS_INLINE void flush(const void *src, uint64_t bytes) {
  const auto *ptr = static_cast<const uint8_t *>(src);
  for (uint64_t i = 0; i < bytes; i += hardware_destructive_interference_size) {
    asm volatile("clflush 0(%0)" ::"r"(ptr + i));
  }
}

} // namespace pnm::memory

#endif
