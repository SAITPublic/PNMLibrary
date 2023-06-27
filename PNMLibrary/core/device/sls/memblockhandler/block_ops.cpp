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

#include "block_ops.h"

#include "core/device/sls/memory_map.h"
#include "core/device/sls/rank_address.h"
#include "core/memory/barrier.h"

#include "common/make_error.h"

#include <linux/sls_resources.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace pnm::sls::device {

// UnsupportedBlockWriter impl.

void UnsupportedBlockWriter::operator()(
    [[maybe_unused]] const memAddr &mem_addr,
    [[maybe_unused]] uint8_t compute_unit, sls_mem_blocks_e block_type,
    [[maybe_unused]] uint32_t offset, [[maybe_unused]] const uint8_t *buf_in,
    [[maybe_unused]] size_t write_size) const {
  // Every write to a readonly block is an error.
  const char *const block_name = sls_mem_blocks_name[block_type];
  throw pnm::error::make_not_sup("'{}'", block_name);
}

// UnsupportedBlockReader impl.

void UnsupportedBlockReader::operator()(
    [[maybe_unused]] const memAddr &mem_addr,
    [[maybe_unused]] uint8_t compute_unit, sls_mem_blocks_e block_type,
    [[maybe_unused]] uint32_t offset, [[maybe_unused]] uint8_t *buf_out,
    [[maybe_unused]] size_t read_size) const {
  // Every read is an error.
  const char *const block_name = sls_mem_blocks_name[block_type];
  throw pnm::error::make_not_sup("'{}'", block_name);
}

// RawBlockWriter impl.

void RawBlockWriter::operator()(const memAddr &mem_addr,
                                [[maybe_unused]] uint8_t compute_unit,
                                sls_mem_blocks_e block_type, uint32_t offset,
                                const uint8_t *buf_in,
                                size_t write_size) const {
  const uint8_t *const begin = buf_in;
  const uint8_t *const end = buf_in + write_size;
  std::copy(begin, end, mem_addr.addr[block_type] + offset);
  mfence();
}

// RankedBlockWriter impl.

void RankedBlockWriter::operator()(const memAddr &mem_addr,
                                   uint8_t compute_unit,
                                   sls_mem_blocks_e block_type, uint32_t offset,
                                   const uint8_t *buf_in,
                                   size_t write_size) const {
  const InterleavedPointer ptr(mem_addr.addr[block_type],
                               rank_to_ha(compute_unit));

  device::memcpy_interleaved(buf_in, write_size, ptr + offset);
}

// RankedBlockReader impl.

void RankedBlockReader::operator()(const memAddr &mem_addr,
                                   [[maybe_unused]] uint8_t compute_unit,
                                   sls_mem_blocks_e block_type, uint32_t offset,
                                   uint8_t *buf_out, size_t read_size) const {
  const InterleavedPointer ptr(mem_addr.addr[block_type],
                               rank_to_ha(compute_unit));
  memcpy_interleaved(ptr + offset, read_size, buf_out);
}

} // namespace pnm::sls::device
