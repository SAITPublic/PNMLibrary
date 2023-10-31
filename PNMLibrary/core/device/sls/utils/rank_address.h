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

#ifndef _SLS_RANK_ADDRESS_H_
#define _SLS_RANK_ADDRESS_H_

#include "common/compiler_internal.h"
#include "hw/pnm_addr_constants.h"

#include <cstdint>

namespace pnm::sls::device {

constexpr auto cunit_to_ha(uint8_t cunit) {
  // 0 (0b000), 2 (0b010), 4 (0b100), etc -- HA 0
  // 1 (0b001), 3 (0b011), 5 (0b101), etc -- HA 1
  return cunit & 0b1;
}

/**@brief Transform rank offset into system offset
 *
 * Function transforms rank address to system address. The transformation
 * process refer to the address schemes presented in conf. page:
 * https://confluence.samsungds.net/display/NP/CXL-SLS
 *
 * The rank address becomes system address when we insert the HA bit in 7
 * position in system address. The HA bit should be corrected by XOR operation
 * with 16 bit in rank address.
 *
 * @param rank_address Offset in rank
 * @param HA HomeAgent that handle rank
 * @return System offset from DAX.start
 */
ALWAYS_INLINE uintptr_t interleaved_address(uintptr_t rank_address,
                                            uint8_t HA) {
  using namespace pnm::hw;

  const auto r16 = (rank_address >> HA_XOR_BIT) & HA_MASK;
  HA = r16 ^ HA;

  static constexpr auto LOW_BIT_MASK = ((1ULL << HA_BIT_POS) - 1);

  const auto low_bits = rank_address & LOW_BIT_MASK;
  const auto hi_bits = rank_address & (~LOW_BIT_MASK);

  auto interleaved_addr =
      (hi_bits << 1) | (static_cast<uintptr_t>(HA) << HA_BIT_POS) | low_bits;

  return interleaved_addr;
}

class InterleavedPointer {
public:
  InterleavedPointer() = default;

  explicit InterleavedPointer(const void *base, uint8_t HA)
      : base_{reinterpret_cast<uint8_t *>(const_cast<void *>(base))}, HA_{HA} {
    update();
  }

  const void *get() const { return interleaved_; }

  void *get() { return interleaved_; }

  template <typename T> const T *as() const {
    return reinterpret_cast<const T *>(interleaved_);
  }

  template <typename T> T *as() { return reinterpret_cast<T *>(interleaved_); }

  InterleavedPointer &operator+=(uintptr_t offset) {
    ranked_addr_ += offset;
    update();
    return *this;
  }

  InterleavedPointer &operator-=(uintptr_t offset) {
    ranked_addr_ -= offset;
    update();
    return *this;
  }

  InterleavedPointer &operator++() {
    (*this) += 1;
    return *this;
  }

  InterleavedPointer operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
  }

  InterleavedPointer &operator--() {
    (*this) -= 1;
    return *this;
  }

  InterleavedPointer operator--(int) {
    auto tmp = *this;
    --(*this);
    return tmp;
  }

  bool operator==(const InterleavedPointer &rhs) const {
    return base_ == rhs.base_ && interleaved_ == rhs.interleaved_;
  }

  bool operator!=(const InterleavedPointer &rhs) const {
    return !(*this == rhs);
  }

  uintptr_t ranked_address() const { return ranked_addr_; }

private:
  void update() {
    interleaved_ = base_ + interleaved_address(ranked_addr_, HA_);
  }

  uint8_t *base_ = nullptr;
  uint8_t HA_ = 0;

  uintptr_t ranked_addr_ = 0;
  uint8_t *interleaved_ = nullptr;
};

inline InterleavedPointer operator+(const InterleavedPointer &p,
                                    uintptr_t offset) {
  InterleavedPointer result = p;
  result += offset;
  return result;
}

inline InterleavedPointer operator-(const InterleavedPointer &p,
                                    uintptr_t offset) {
  InterleavedPointer result = p;
  result -= offset;
  return result;
}

inline InterleavedPointer operator+(uintptr_t offset,
                                    const InterleavedPointer &p) {
  return p + offset;
}

uint64_t memcpy_interleaved(const void *src, uint64_t bytes,
                            InterleavedPointer dst);

uint64_t memcpy_interleaved(InterleavedPointer src, uint64_t bytes, void *dst);

/**@brief Transform pointer to InterleavedPointer for specified cunit*/
InterleavedPointer make_interleaved_pointer(uint32_t cunit,
                                            uint64_t phys_offset,
                                            const uint8_t *virtual_addr);

} // namespace pnm::sls::device

#endif /* _SLS_RANK_ADDRESS_H_ */
