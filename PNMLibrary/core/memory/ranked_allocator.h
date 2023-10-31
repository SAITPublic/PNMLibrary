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

#ifndef _RANKED_ALLOCATOR_H_
#define _RANKED_ALLOCATOR_H_

#include "pnmlib/core/allocator.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/properties.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace pnm::memory {

// [TODO: MCS23-1130] Finalize untying after kernel changes
/** @brief Ranked memory allocator.
 *
 * The class implements ranked memory allocation logic.
 *
 * */
class RankedAllocator : public Allocator {
public:
  using RegionType = RankedRegion;

  explicit RankedAllocator(const Device *dev, size_t regions_count)
      : device_{dev}, regions_count_(regions_count) {}

private:
  DeviceRegion
  allocate_impl(uint64_t size,
                const pnm::property::PropertiesList &props) override;

  void allocate_single(uint64_t size,
                       const pnm::property::PropertiesList &props,
                       RegionType &region);

  void allocate_replicate_all(uint64_t size,
                              const pnm::property::PropertiesList &props,
                              RegionType &region);

  SequentialRegion allocate_ioctl(uint64_t size, std::optional<uint8_t> cunit);

  void deallocate_impl(const DeviceRegion &region) override;

  void deallocate_ioctl(const SequentialRegion &seq_region);

  DeviceRegion share_region_impl(const DeviceRegion &region) override;

  DeviceRegion get_shared_alloc_impl(const DeviceRegion &region) override;

  DeviceRegion sharing_ioctl(const DeviceRegion &region, uint64_t cmd);

  const Device *device_;
  const uint64_t regions_count_;
};

} // namespace pnm::memory

#endif //_RANKED_ALLOCATOR_H_
