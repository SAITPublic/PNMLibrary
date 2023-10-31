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

#include "sequential_allocator.h"

#include "common/make_error.h"
#include "common/topology_constants.h"

#include "pnmlib/core/allocator.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/properties.h"

#include <linux/imdb_resources.h>

#include <linux/pnm_resources.h>
#include <sys/ioctl.h>

#include <cassert>
#include <cstdint>

using pnm::imdb::device::topo;

namespace pnm::memory {
DeviceRegion SequentialAllocator::allocate_impl(
    uint64_t size,
    [[maybe_unused]] const pnm::property::PropertiesList &props) {
  pnm_allocation req{
      .addr = 0,
      .size = size,
      .memory_pool = get_alloc_preference(props),
      .is_global = 0,
  };

  if (ioctl(device_->get_resource_fd(), IMDB_IOCTL_ALLOCATE, &req) < 0) {
    throw pnm::error::make_oom("Unable to allocate {} bytes.", size);
  }

  RegionType region{
      .start = req.addr,
      .size = req.size,
      .location = req.memory_pool,
      .is_global = req.is_global,
  };

  return region;
}

void SequentialAllocator::deallocate_impl(const DeviceRegion &region) {
  const auto &seq_region = std::get<RegionType>(region);

  pnm_allocation req{
      .addr = seq_region.start,
      .size = seq_region.size,
      .memory_pool = seq_region.location.value(),
      .is_global = seq_region.is_global,
  };

  if (ioctl(device_->get_resource_fd(), IMDB_IOCTL_DEALLOCATE, &req) < 0) {
    throw pnm::error::make_fail(
        "Unable to deallocate memory region from {}, size {}.", req.addr,
        req.size);
  }
}

DeviceRegion
SequentialAllocator::share_region_impl(const DeviceRegion &region) {
  return sharing_ioctl(region, IMDB_IOCTL_MAKE_SHARED_ALLOC);
}

DeviceRegion
SequentialAllocator::get_shared_alloc_impl(const DeviceRegion &region) {
  return sharing_ioctl(region, IMDB_IOCTL_GET_SHARED_ALLOC);
}

DeviceRegion SequentialAllocator::sharing_ioctl(const DeviceRegion &region,
                                                uint64_t cmd) {
  const auto &seq_region = std::get<RegionType>(region);

  if (!seq_region.location.has_value()) {
    throw pnm::error::make_fail(
        "Unable to share memory region from {}, size {}. Invalid pool.",
        seq_region.start, seq_region.size);
  }

  pnm_allocation req{
      .addr = seq_region.start,
      .size = seq_region.size,
      .memory_pool = *seq_region.location,
      .is_global = seq_region.is_global,
  };

  if (ioctl(device_->get_resource_fd(), cmd, &req) < 0) {
    throw pnm::error::make_fail(
        "Unable to share memory region from {}, size {}.", req.addr, req.size);
  }

  SequentialRegion shared_region{
      .start = req.addr,
      .size = req.size,
      .location = req.memory_pool,
      .is_global = req.is_global,
  };

  return shared_region;
}

uint8_t SequentialAllocator::get_alloc_preference(
    const pnm::property::PropertiesList &props) const {
  uint8_t preference = topo().NumOfRanks; // any most free rank

  assert(!props.has_property<property::AllocPolicy>() &&
         "Allocation policies are not supported");

  if (props.has_property<property::CURegion>()) {
    preference = props.get_property<property::CURegion>().cunit();
  }

  return preference;
}
} // namespace pnm::memory
