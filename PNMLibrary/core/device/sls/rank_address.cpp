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

#include "rank_address.h"

#include "core/memory/barrier.h"

#include "common/topology_constants.h"

#include <algorithm>
#include <cstdint>

namespace pnm::sls::device {

using pnm::device::topo;

namespace {
uint64_t memcpy_block(const uint8_t *&source, uint64_t block_size,
                      InterleavedPointer &dst) {
  std::copy_n(source, block_size, dst.as<uint8_t>());
  if constexpr (PNM_PLATFORM != FUNCSIM) {
    flush(dst.get(), block_size);
    mfence();
  }
  dst += block_size;
  source += block_size;
  return block_size;
}

uint64_t memcpy_block(InterleavedPointer &source, uint64_t block_size,
                      uint8_t *&dst) {
  std::copy_n(source.as<uint8_t>(), block_size, dst);
  source += block_size;
  dst += block_size;
  return block_size;
}
} // namespace

uint64_t memcpy_interleaved(const void *src, uint64_t bytes,
                            InterleavedPointer dst) {
  const auto *source = static_cast<const uint8_t *>(src);
  uint64_t copied = 0;

  if (dst.ranked_address() % topo().RankInterleavingSize != 0) {
    auto block_size =
        std::min(bytes, topo().RankInterleavingSize -
                            dst.ranked_address() % topo().RankInterleavingSize);
    copied += memcpy_block(source, block_size, dst);
  }

  while (copied < bytes) {
    auto block_size = std::min(topo().RankInterleavingSize, bytes - copied);
    copied += memcpy_block(source, block_size, dst);
  }

  return copied;
}

uint64_t memcpy_interleaved(InterleavedPointer src, uint64_t bytes, void *dst) {
  auto *destination = static_cast<uint8_t *>(dst);
  uint64_t copied = 0;

  if (src.ranked_address() % topo().RankInterleavingSize != 0) {
    const auto block_size =
        std::min(bytes, topo().RankInterleavingSize -
                            src.ranked_address() % topo().RankInterleavingSize);
    copied += memcpy_block(src, block_size, destination);
  }

  while (copied < bytes) {
    auto block_size = std::min(topo().RankInterleavingSize, bytes - copied);
    copied += memcpy_block(src, block_size, destination);
  }

  return copied;
}

pnm::sls::device::InterleavedPointer
make_interleaved_pointer(uint32_t rank, uint64_t phys_offset,
                         const uint8_t *virtual_addr) {
  const auto HA = pnm::sls::device::rank_to_ha(rank);
  // We should start address transformation from the channel start address
  // It is ok, until we do not dereference the `base` pointer
  const auto *base =
      virtual_addr - pnm::sls::device::interleaved_address(phys_offset, HA);

  return pnm::sls::device::InterleavedPointer(base, HA) + phys_offset;
}

} // namespace pnm::sls::device
