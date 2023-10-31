/*
 *  Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 *  This software is proprietary of Samsung Electronics.
 *  No part of this software, either material or conceptual may be copied or
 *  distributed, transmitted, transcribed, stored in a retrieval system or
 *  translated into any human or computer language in any form by any means,
 *  electronic, mechanical, manual or otherwise, or disclosed to third parties
 *  without the express written permission of Samsung Electronics.
 */

#include "accessor.h"

#include "core/device/sls/utils/rank_address.h"

#include "common/mapped_file.h"

#include "pnmlib/sls/control.h"

#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/views.h"

#include <linux/sls_resources.h>

#include <cassert>
#include <cstdint>
#include <cstring>

namespace {
inline constexpr auto MMAP_ALIGN = 4096UL;
inline constexpr auto ALIGN_MASK = MMAP_ALIGN - 1;

uint64_t aligned_floor(uint64_t offset) { return offset & (~ALIGN_MASK); }

uint64_t aligned_ceil(uint64_t offset) {
  return aligned_floor(offset + ALIGN_MASK);
}
} // namespace

namespace pnm::memory {

// size field is unused due to WC caching policy
const uint8_t *
SlsAccessorCore::access_impl(uint64_t byte_offset,
                             [[maybe_unused]] uint64_t size) const {
  for (const auto &phys_region : region_.regions) {
    if (phys_region.location.has_value()) {
      // Here we assume that data in all ranks are same
      auto iptr = pnm::sls::device::make_interleaved_pointer(
          *phys_region.location, phys_region.start,
          vregion_[*phys_region.location].begin());
      return (iptr + byte_offset).as<uint8_t>();
    }
  }

  throw pnm::error::InvalidArguments("Access an empty region.");
}

VirtualRegion SlsAccessorCore::virtual_range_impl() const { return vregion_; }

DeviceRegion SlsAccessorCore::phys_range_impl() const { return region_; }

uint64_t SlsAccessorCore::bytes_impl() const { return data_size_; }

SlsAccessorCore::SlsAccessorCore(const DeviceRegion &region,
                                 DevicePointer &device)
    : device_id_{device->get_devmem_fd()},
      region_(std::get<RegionType>(region)) {
  init_virtual_region();
}

void SlsAccessorCore::init_virtual_region() {
  vregion_data_.resize(region_.regions.size());
  vregion_.resize(region_.regions.size());

  const pnm::sls::device::Control ctrl_context;
  const auto mem_info = ctrl_context.get_mem_info();

  for (size_t i = 0; i < region_.regions.size(); ++i) {
    const SequentialRegion &sreg = region_.regions[i];
    if (!sreg.location.has_value()) {
      vregion_data_[i] = pnm::utils::MappedData();
      vregion_[i] = pnm::views::common<uint8_t>{};
      continue;
    }

    if (data_size_ == 0) {
      data_size_ = sreg.size;
    }

    assert(data_size_ == sreg.size &&
           "Size in RankedRegion should be equal in all dimensions");

    auto offset = pnm::sls::device::interleaved_address(
        sreg.start, pnm::sls::device::cunit_to_ha(*sreg.location));
    auto size = pnm::sls::device::interleaved_address(
                    sreg.start + sreg.size,
                    pnm::sls::device::cunit_to_ha(*sreg.location)) -
                offset;

    offset += mem_info[*sreg.location][SLS_BLOCK_BASE].map_offset;

    vregion_data_[i] = pnm::utils::MappedData(device_id_, ::aligned_ceil(size),
                                              "address from phys. to virt.",
                                              ::aligned_floor(offset));

    auto *addr = static_cast<uint8_t *>(vregion_data_[i].data()) +
                 (offset & ::ALIGN_MASK);
    vregion_[i] = pnm::views::common<uint8_t>{addr, addr + size};
  }
}

void SlsAccessorCore::store_impl(const void *value, uint64_t size,
                                 uint64_t byte_offset) {
  for (const auto &phys_reg : region_.regions) {
    if (phys_reg.location.has_value()) {
      auto iptr = pnm::sls::device::make_interleaved_pointer(
          *phys_reg.location, phys_reg.start,
          vregion_[*phys_reg.location].begin());

      pnm::sls::device::memcpy_interleaved(value, size, iptr + byte_offset);
    }
  }
}
} // namespace pnm::memory
