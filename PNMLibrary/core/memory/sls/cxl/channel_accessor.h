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

#ifndef PNM_CHANNEL_ACCESSOR_H
#define PNM_CHANNEL_ACCESSOR_H

#include "common/mapped_file.h"

#include "pnmlib/core/accessor.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/views.h"

#include <cstdint>
#include <vector>

namespace pnm::memory {
class ChannelAccessorCore : public AccessorCore {
public:
  ChannelAccessorCore(const DeviceRegion &region, DevicePointer &device);

  ChannelAccessorCore(const ChannelAccessorCore &other) = default;
  ChannelAccessorCore(ChannelAccessorCore &&other) noexcept = default;

  ChannelAccessorCore &operator=(const ChannelAccessorCore &other) = default;
  ChannelAccessorCore &operator=(ChannelAccessorCore &&other) = default;

private:
  using RegionType = RankedRegion;
  using VRegionType = VirtualRankedRegion;

  const uint8_t *access_impl(uint64_t byte_offset,
                             uint64_t size) const override;

  VirtualRegion virtual_range_impl() const override;

  DeviceRegion phys_range_impl() const override;

  uint64_t bytes_impl() const override;

  void store_impl(const void *value, uint64_t size,
                  uint64_t byte_offset) override;

  pnm::views::common<uint8_t>
  mmap_region(const SequentialRegion &seq_region) const;

  pnm::Device *device_ = nullptr;

  RegionType region_;
  VirtualRankedRegion virtual_region_;
  std::vector<pnm::utils::MappedData> vregion_data_;

  std::vector<uint64_t> base_offsets_;

  uint64_t data_size_ = 0; // Data size in bytes for single channel
};
} // namespace pnm::memory

#endif // PNM_CHANNEL_ACCESSOR_H
