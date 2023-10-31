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

#ifndef _SLS_MEMBLOCKHANDLER_BLOCK_OPS_H_
#define _SLS_MEMBLOCKHANDLER_BLOCK_OPS_H_

#include "core/device/sls/utils/memory_map.h"

#include "common/memory/barrier.h"

#include <linux/sls_resources.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace pnm::sls::device {

/** @brief Block writer that throws an error because the block is
 * read-only.
 */
struct UnsupportedBlockWriter {
  void operator()(const memAddr &mem_addr, uint8_t compute_unit,
                  sls_mem_blocks_e block_type, uint32_t offset,
                  const uint8_t *buf_in, size_t write_size) const;
};

/** @brief Block reader that throws pnm not supported exception. */
struct UnsupportedBlockReader {
  void operator()(const memAddr &mem_addr, uint8_t compute_unit,
                  sls_mem_blocks_e block_type, uint32_t offset,
                  uint8_t *buf_out, size_t read_size) const;
};

/** @brief Block writer that uses raw memory copying. */
struct RawBlockWriter {
  void operator()(const memAddr &mem_addr, uint8_t compute_unit,
                  sls_mem_blocks_e block_type, uint32_t offset,
                  const uint8_t *buf_in, size_t write_size) const;
};

/** @brief Block reader that uses raw memory copying. */
template <bool FLUSH_BEFORE = false> struct RawBlockReader {
  void operator()(const memAddr &mem_addr,
                  [[maybe_unused]] uint8_t compute_unit,
                  sls_mem_blocks_e block_type, uint32_t offset,
                  uint8_t *buf_out, size_t read_size) const {
    const uint8_t *const begin = mem_addr.addr[block_type] + offset;
    const uint8_t *const end = begin + read_size;

    if constexpr (FLUSH_BEFORE) {
      pnm::memory::flush(begin, read_size);
    }

    std::copy(begin, end, buf_out);
    pnm::memory::mfence();
  }
};

/** @brief Block writer that uses rank memory access with interleaving. */
struct RankedBlockWriter {
  void operator()(const memAddr &mem_addr, uint8_t compute_unit,
                  sls_mem_blocks_e block_type, uint32_t offset,
                  const uint8_t *buf_in, size_t write_size) const;
};

/** @brief Block reader that uses rank memory access with interleaving. */
struct RankedBlockReader {
  void operator()(const memAddr &mem_addr, uint8_t compute_unit,
                  sls_mem_blocks_e block_type, uint32_t offset,
                  uint8_t *buf_out, size_t read_size) const;
};

} // namespace pnm::sls::device

#endif // _SLS_MEMBLOCKHANDLER_BLOCK_OPS_H_
