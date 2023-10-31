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

#include "channel_accessor.h"

#include "common/mapped_file.h"

#include "pnmlib/sls/control.h"

#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/misc_utils.h"
#include "pnmlib/common/views.h"

#include <linux/sls_resources.h>

#include <algorithm>
#include <cstdint>

namespace {
auto page_size = 2UL << 20; // 2MB because DAX uses huge page by default
} // namespace

namespace pnm::memory {
ChannelAccessorCore::ChannelAccessorCore(const DeviceRegion &region,
                                         DevicePointer &device)
    : device_{device.get()}, region_{std::get<RegionType>(region)},
      virtual_region_(region_.regions.size()),
      vregion_data_(virtual_region_.size()),
      base_offsets_(virtual_region_.size()) {

  const pnm::sls::device::Control ctrl_context;
  const auto mem_info = ctrl_context.get_mem_info();

  for (auto i = 0UL; i < virtual_region_.size(); ++i) {
    base_offsets_[i] = mem_info[i][SLS_BLOCK_BASE].map_offset;
  }

  for (auto i = 0U; i < virtual_region_.size(); ++i) {
    if (region_.regions[i].location.has_value()) {
      vregion_data_[i] = pnm::utils::MappedData(
          device_->get_devmem_fd(),
          pnm::utils::fast_align_up(region_.regions[i].size, ::page_size),
          "device",
          region_.regions[i].start +
              base_offsets_[*region_.regions[i].location]);
      virtual_region_[i] =
          vregion_data_[i].get_view<uint8_t>(region_.regions[i].size);
      data_size_ = region_.regions[i].size;
    }
  }
}

// size field is unused due to WC caching policy
const uint8_t *
ChannelAccessorCore::access_impl(uint64_t byte_offset,
                                 [[maybe_unused]] uint64_t size) const {
  for (const auto &vchannel_region : virtual_region_) {
    if (!vchannel_region.empty()) {
      // Here we assume that data are same in all channels
      return vchannel_region.data() + byte_offset;
    }
  }

  throw pnm::error::InvalidArguments("Access an empty region.");
}

VirtualRegion ChannelAccessorCore::virtual_range_impl() const {
  return virtual_region_;
}

DeviceRegion ChannelAccessorCore::phys_range_impl() const { return region_; }

uint64_t ChannelAccessorCore::bytes_impl() const { return data_size_; }

void ChannelAccessorCore::store_impl(const void *value, uint64_t size,
                                     uint64_t byte_offset) {
  for (auto vchannel_region : virtual_region_) {
    if (!vchannel_region.empty()) {
      std::copy_n(static_cast<const uint8_t *>(value), size,
                  vchannel_region.data() + byte_offset);
    }
  }
}

} // namespace pnm::memory
