// nbuffer_memory.cpp - sls n-buffered memory support --------------//

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

#include "nbuffer_memory.h"

#include "memory_map.h"
#include "rank_address.h"

#include "common/topology_constants.h"

#include <linux/sls_resources.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace pnm::sls::device {

void NBufferedMemory::check_index_access(
    [[maybe_unused]] uint8_t rank,
    [[maybe_unused]] NBufferedMemory::Type type) {
  assert(rank < pnm::device::topo().NumOfRanks);
  assert(type >= NBufferedMemory::Type::READ &&
         type < NBufferedMemory::Type::MAX);
}

void NBufferedMemory::check_index_value([[maybe_unused]] uint8_t value) const {
  assert(value >= 0 && value < buffers_num_);
}

NBufferedMemory::NBufferedMemory(MemMap &memory, size_t single_buffer_size,
                                 size_t buffers_num,
                                 sls_mem_blocks_e mem_block_type)
    : m_map_(memory), single_buffer_size_{single_buffer_size},
      buffers_num_{buffers_num},
      indices_(pnm::device::topo().NumOfRanks, std::vector<uint8_t>(Type::MAX)),
      mem_block_type_{mem_block_type} {
  assert(buffers_num_ > 0);
  assert(single_buffer_size_ > 0);

  for (uint8_t rank = 0; rank < pnm::device::topo().NumOfRanks; ++rank) {
    set_index(rank, Type::READ, buffers_num_ - 1);
    set_index(rank, Type::WRITE, 0);
  }
}

pnm::sls::device::InterleavedPointer
NBufferedMemory::get_buffer(uint8_t rank, NBufferedMemory::Type type) const {
  return get_buffer_by_index(rank, get_index(rank, type));
}

pnm::sls::device::InterleavedPointer
NBufferedMemory::get_buffer_by_index(uint8_t rank, uint8_t index) const {
  return InterleavedPointer(m_map_[rank].addr[mem_block_type_],
                            rank_to_ha(rank)) +
         single_buffer_size_ * index;
}

void NBufferedMemory::shift_buffers(uint8_t rank) {
  shift_index(rank, Type::READ);
  shift_index(rank, Type::WRITE);
}

uint8_t NBufferedMemory::get_index(uint8_t rank, Type type) const {
  check_index_access(rank, type);
  const auto idx = indices_[rank][type];
  check_index_value(idx);
  return idx;
}

void NBufferedMemory::set_index(uint8_t rank, Type type, uint8_t value) {
  check_index_access(rank, type);
  check_index_value(value);
  indices_[rank][type] = value;
}

void NBufferedMemory::shift_index(uint8_t rank, Type type) {
  set_index(rank, type, (get_index(rank, type) + 1) % buffers_num_);
}

} // namespace pnm::sls::device
