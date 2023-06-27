/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
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

#ifndef PNM_COMPILER_INTERNAL_H
#define PNM_COMPILER_INTERNAL_H

#define ALWAYS_INLINE __attribute__((always_inline)) inline
#if defined(__clang__)
#define NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW                                  \
  __attribute__((no_sanitize("unsigned-integer-overflow")))
#if __clang_major__ >= 12
#define NO_SANITIZE_UNSIGNED_SHIFT_BASE                                        \
  __attribute__((no_sanitize("unsigned-shift-base")))
#else
#define NO_SANITIZE_UNSIGNED_SHIFT_BASE
#endif // __clang_major__ >= 12
#elif defined(__GNUC__)
#define NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW
#define NO_SANITIZE_UNSIGNED_SHIFT_BASE
#endif // defined(__clang__)

#if defined(__clang__)
#define TSAN __has_feature(thread_sanitizer)
#elif defined(__GNUC__) // defined(__clang__)
#if defined(__SANITIZE_THREAD__)
#define TSAN 1
#else // defined(__SANITIZE_THREAD__)
#define TSAN 0
#endif // defined(__SANITIZE_THREAD__)
#endif // defined(__clang__)

#define LIKELY(x)                                                              \
  __builtin_expect(!!(x), 1) // NOLINT(readability-simplify-boolean-expr)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#ifdef __cpp_lib_hardware_interference_size
#include <new>

// Normally using this inside a header triggers an error because headers might
// be exported. Since this value is dependent on the compilation environment, it
// really shouldn't be exported. In our case however this header is only used
// internally, so we can safely ignore this warning
//
// NOTE: We use pragma to disable this warning only for compilers that support
// this in the first place
#pragma GCC diagnostic ignored "-Winterference-size"

inline constexpr auto hardware_destructive_interference_size =
    std::hardware_destructive_interference_size;
#else  // __cpp_lib_hardware_interference_size
// 64 bytes on x86-64
inline constexpr auto hardware_destructive_interference_size = 64U;
#endif // __cpp_lib_hardware_interference_size

#define CACHE_ALIGNED alignas(hardware_destructive_interference_size)

#if TSAN
#include <sanitizer/tsan_interface.h>

namespace pnm::utils {
/* TSan virtual mutex */
class tsan_mutex {
public:
  void lock() { __tsan_acquire(this); }
  void unlock() { __tsan_release(this); }
};

} // namespace pnm::utils

#else // TSAN

namespace pnm::utils {
/* TSan virtual mutex */
class tsan_mutex {
public:
  void lock() {}
  void unlock() {}
};

} // namespace pnm::utils

#endif // TSAN

#endif /* PNM_COMPILER_INTERNAL_H */
