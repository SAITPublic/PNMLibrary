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

#ifndef SLS_SECURE_IO_H
#define SLS_SECURE_IO_H

#include "secure/encryption/io_traits.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iterator>

namespace pnm::sls::secure {
class TrivialMemoryReader {
public:
  explicit TrivialMemoryReader(const void *ptr)
      : ptr_{static_cast<const char *>(ptr)} {}

  void read(char *dst, uint64_t size) {
    std::copy_n(ptr_, size, dst);
    std::advance(ptr_, size);
  }

private:
  const char *ptr_;
};

class TrivialMemoryWriter {
public:
  explicit TrivialMemoryWriter(void *dst) : ptr_{static_cast<char *>(dst)} {}

  void write(const char *src, uint64_t size) {
    ptr_ = std::copy_n(src, size, ptr_);
  }

private:
  char *ptr_;
};

struct MemoryWriter : public TrivialMemoryWriter {
  using TrivialMemoryWriter::TrivialMemoryWriter;
};

template <>
struct io_traits<MemoryWriter>
    : public io_traits_base<MemoryWriter, io_traits<MemoryWriter>> {
  static constexpr auto meta_size = 64; // TAG_PHYSICAL_SIZE, specified by HW
  static void write_meta(MemoryWriter &out, const char *data, uint64_t size) {
    assert(size <= meta_size && "Something wrong with meta (tag) size");

    static constexpr std::array<char, meta_size> padding{};
    out.write(data, size);
    out.write(padding.data(), meta_size - size);
  }
};

} // namespace pnm::sls::secure

#endif // SLS_SECURE_IO_H
