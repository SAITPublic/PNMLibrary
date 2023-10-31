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

#include "ranked_allocator.h"

#include "common/error_message.h"
#include "common/make_error.h"

#include "pnmlib/core/allocator.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/properties.h"

#include <linux/sls_resources.h>

#include <linux/pnm_resources.h>
#include <sys/ioctl.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace pnm::memory {
DeviceRegion
RankedAllocator::allocate_impl(uint64_t size,
                               const pnm::property::PropertiesList &props) {
  RegionType region{
      std::vector<SequentialRegion>(regions_count_, SequentialRegion{})};

  auto policy = props.has_property<property::AllocPolicy>()
                    ? static_cast<sls_user_preferences>(
                          props.get_property<property::AllocPolicy>().policy())
                    : SLS_ALLOC_AUTO;

  switch (policy) {
  case SLS_ALLOC_AUTO:
  case SLS_ALLOC_SINGLE:
    allocate_single(size, props, region);
    return region;
  case SLS_ALLOC_REPLICATE_ALL:
    allocate_replicate_all(size, props, region);
    return region;
  case SLS_ALLOC_DISTRIBUTE_ALL:
    throw pnm::error::NotSupported(
        "SLS_ALLOC_DISTRIBUTE_ALL policy unimplemented.");
  }

  throw pnm::error::make_inval(
      "Unknown AllocPolicy. Got = {}.",
      props.get_property<property::AllocPolicy>().policy());
}

void RankedAllocator::allocate_single(
    uint64_t size, const pnm::property::PropertiesList &props,
    RankedAllocator::RegionType &region) {
  auto cunit =
      props.has_property<property::CURegion>()
          ? std::optional{props.get_property<property::CURegion>().cunit()}
          : std::nullopt;
  auto seq_region = allocate_ioctl(size, cunit);
  region.regions[*seq_region.location] = seq_region;
}

void RankedAllocator::allocate_replicate_all(
    uint64_t size, const pnm::property::PropertiesList &props,
    RankedAllocator::RegionType &region) {
  if (props.has_property<property::CURegion>()) {
    throw pnm::error::InvalidArguments(
        "Cannot specify CURegion with SLS_ALLOC_REPLICATE_ALL.");
  }

  for (uint8_t rank = 0; rank < regions_count_; ++rank) {
    try {
      auto seq_region = allocate_ioctl(size, rank);
      region.regions[*seq_region.location] = seq_region;
    } catch (...) {
      deallocate_impl(region);
      throw;
    }
  }
}

void RankedAllocator::deallocate_impl(const DeviceRegion &region) {
  const auto &typed_region = std::get<RegionType>(region);

  for (const auto &seq_region : typed_region.regions) {
    if (seq_region.size > 0) {
      deallocate_ioctl(seq_region);
    }
  }
}

void RankedAllocator::deallocate_ioctl(const SequentialRegion &seq_region) {
  const pnm_allocation request{
      .addr = seq_region.start,
      .size = seq_region.size,
      .memory_pool = seq_region.location.value(),
      .is_global = seq_region.is_global,
  };

  if (ioctl(device_->get_resource_fd(), DEALLOCATE_MEMORY, &request) < 0) {
    pnm::error::throw_from_errno(errno, pnm::error::ioctl_from_errno);
  }
}

SequentialRegion RankedAllocator::allocate_ioctl(uint64_t size,
                                                 std::optional<uint8_t> cunit) {
  pnm_allocation request{
      .addr = 0,
      .size = size,
      .memory_pool = cunit.value_or(SLS_ALLOC_ANY_CUNIT),
      .is_global = 0,
  };

  if (ioctl(device_->get_resource_fd(), ALLOCATE_MEMORY, &request) < 0) {
    pnm::error::throw_from_errno(errno);
  }

  return SequentialRegion{
      .start = request.addr,
      .size = size,
      .location = request.memory_pool,
      .is_global = request.is_global,
  };
}

DeviceRegion RankedAllocator::share_region_impl(const DeviceRegion &region) {
  return sharing_ioctl(region, MAKE_SHARED_ALLOC);
}

DeviceRegion
RankedAllocator::get_shared_alloc_impl(const DeviceRegion &region) {
  return sharing_ioctl(region, GET_SHARED_ALLOC);
}

DeviceRegion RankedAllocator::sharing_ioctl(const DeviceRegion &region,
                                            uint64_t cmd) {
  const auto &ranked_region = std::get<RegionType>(region);
  const auto amount_of_regions = ranked_region.regions.size();

  RegionType shared_region{
      std::vector<SequentialRegion>(amount_of_regions, SequentialRegion{})};

  for (size_t i = 0; i < amount_of_regions; ++i) {
    const auto &seq_region = ranked_region.regions[i];

    if (!seq_region.location.has_value()) {
      continue;
    }

    pnm_allocation request{
        .addr = seq_region.start,
        .size = seq_region.size,
        .memory_pool = *seq_region.location,
        .is_global = seq_region.is_global,
    };

    if (ioctl(device_->get_resource_fd(), cmd, &request) < 0) {
      pnm::error::throw_from_errno(errno);
    }

    const SequentialRegion seq_shared_region{
        .start = request.addr,
        .size = request.size,
        .location = request.memory_pool,
        .is_global = request.is_global,
    };

    shared_region.regions[i] = seq_shared_region;
  }

  return shared_region;
}
} // namespace pnm::memory
