
/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#ifndef SLS_TEST_EXTRA_H
#define SLS_TEST_EXTRA_H

#include "secure/encryption/sls_engine.h"
#include "secure/encryption/sls_mac.h"

#include "common/sls_types.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

// Manual reader from memory that can be reset.
struct PtrReader {
  explicit PtrReader(const void *ptr) : ptr_{static_cast<const char *>(ptr)} {}
  void read(char *mem, size_t size) {
    std::copy_n(ptr_ + position_, size, mem);
    position_ += size;
  }
  void reset() { position_ = 0; }

private:
  const char *ptr_;
  size_t position_{};
};

struct PtrWriter {
  explicit PtrWriter(void *ptr) : ptr_{static_cast<char *>(ptr)} {}
  void write(const char *mem, size_t size) {
    std::copy_n(mem, size, ptr_);
    ptr_ += size;
  }

private:
  char *ptr_;
};

template <typename T = uint32_t> inline auto create_encryption_engine() {
  const char tkey[] = "123456789012345";
  pnm::types::FixedVector<uint8_t, 16> key{};
  std::copy(tkey, tkey + sizeof(tkey), key.data());

  return std::pair{pnm::sls::secure::EncryptionEngine<T>(42, key),
                   pnm::sls::secure::VerificationEngine(0x69, 42, key)};
}

#endif // SLS_TEST_EXTRA_H
