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

#include "sequential_accessor.h"

#include "common/memory/barrier.h"

#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/misc_utils.h"

#include <cassert>
#include <cstdint>
#include <cstring>

pnm::memory::SequentialAccessorCore::SequentialAccessorCore(
    const pnm::memory::DeviceRegion &region, pnm::DevicePointer &device,
    uint64_t address_align)
    : region_{std::get<SequentialRegion>(region)},
      address_align_(address_align),
      device_map_(device->get_devmem_fd(),
                  pnm::utils::fast_align_up(region_.size, address_align_),
                  "device memory", region_.start),
      virtual_region_{device_map_.get_view<uint8_t>(region_.size)} {
  assert(region_.start % address_align_ == 0 && "Unaligned address");
}

const uint8_t *
pnm::memory::SequentialAccessorCore::access_impl(uint64_t byte_offset,
                                                 uint64_t size) const {
  if constexpr (PNM_PLATFORM != FUNCSIM) {
    pnm::memory::flush(virtual_region_.data() + byte_offset, size);
    pnm::memory::mfence();
  }
  return virtual_region_.data() + byte_offset;
}

pnm::memory::VirtualRegion
pnm::memory::SequentialAccessorCore::virtual_range_impl() const {
  return virtual_region_;
}

pnm::memory::DeviceRegion
pnm::memory::SequentialAccessorCore::phys_range_impl() const {
  return region_;
}

uint64_t pnm::memory::SequentialAccessorCore::bytes_impl() const {
  return virtual_region_.size();
}

void pnm::memory::SequentialAccessorCore::store_impl(const void *value,
                                                     uint64_t size,
                                                     uint64_t byte_offset) {
  std::memcpy(virtual_region_.data() + byte_offset, value, size);
  if constexpr (PNM_PLATFORM != FUNCSIM) {
    pnm::memory::flush(virtual_region_.data() + byte_offset, size);
    pnm::memory::mfence();
  }
}
