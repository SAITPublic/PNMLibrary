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

#include "pnmlib/core/allocator.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/properties.h"

#include <linux/imdb_resources.h>

#include <sys/ioctl.h>

#include <cassert>
#include <cstdint>

pnm::memory::DeviceRegion pnm::memory::SequentialAllocator::allocate_impl(
    uint64_t size,
    [[maybe_unused]] const pnm::property::PropertiesList &props) {
  imdb_allocation req{
      .address = 0,
      .size = size,
      .rank = get_alloc_preference(props),
  };

  if (ioctl(device_->get_resource_fd(), IMDB_IOCTL_ALLOCATE, &req) < 0) {
    throw pnm::error::make_oom("Unable to allocate {} bytes.", size);
  }

  RegionType region{
      .start = req.address,
      .size = req.size,
      .location = static_cast<int32_t>(req.rank),
  };

  return region;
}

void pnm::memory::SequentialAllocator::deallocate_impl(
    const DeviceRegion &region) {
  const auto &seq_region = std::get<RegionType>(region);

  imdb_allocation req{
      .address = seq_region.start,
      .size = seq_region.size,
      .rank = static_cast<uint8_t>(seq_region.location),
  };
  if (ioctl(device_->get_resource_fd(), IMDB_IOCTL_DEALLOCATE, &req) < 0) {
    throw pnm::error::make_fail(
        "Unable to deallocate memory region from {}, size {}.", req.address,
        req.size);
  }
}

uint8_t pnm::memory::SequentialAllocator::get_alloc_preference(
    const pnm::property::PropertiesList &props) const {
  uint8_t preference = IMDB_NUM_OF_RANK; // any most free rank

  assert(!props.has_property<property::AllocPolicy>() &&
         "Allocation policies are not supported");

  if (props.has_property<property::CURegion>()) {
    preference = props.get_property<property::CURegion>().rank();
  }

  return preference;
}
