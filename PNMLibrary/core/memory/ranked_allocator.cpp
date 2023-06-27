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

#include <sys/ioctl.h>

#include <cerrno>
#include <cstdint>
#include <optional>
#include <vector>

pnm::memory::DeviceRegion pnm::memory::RankedAllocator::allocate_impl(
    uint64_t size, const pnm::property::PropertiesList &props) {
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

void pnm::memory::RankedAllocator::allocate_single(
    uint64_t size, const pnm::property::PropertiesList &props,
    pnm::memory::RankedAllocator::RegionType &region) {
  auto rank =
      props.has_property<property::CURegion>()
          ? std::optional{props.get_property<property::CURegion>().rank()}
          : std::nullopt;
  auto seq_region = allocate_ioctl(size, rank);
  region.regions[seq_region.location] = seq_region;
}

void pnm::memory::RankedAllocator::allocate_replicate_all(
    uint64_t size, const pnm::property::PropertiesList &props,
    pnm::memory::RankedAllocator::RegionType &region) {
  if (props.has_property<property::CURegion>()) {
    throw pnm::error::InvalidArguments(
        "Cannot specify CURegion with SLS_ALLOC_REPLICATE_ALL.");
  }

  for (uint32_t rank = 0; rank < regions_count_; ++rank) {
    try {
      auto seq_region = allocate_ioctl(size, rank);
      region.regions[seq_region.location] = seq_region;
    } catch (...) {
      deallocate_impl(region);
      throw;
    }
  }
}

void pnm::memory::RankedAllocator::deallocate_impl(
    const pnm::memory::DeviceRegion &region) {
  const auto &typed_region = std::get<RegionType>(region);

  for (const auto &seq_region : typed_region.regions) {
    if (seq_region.size > 0) {
      deallocate_ioctl(seq_region);
    }
  }
}

void pnm::memory::RankedAllocator::deallocate_ioctl(
    const SequentialRegion &seq_region) {
  const sls_memory_alloc_request request{
      .rank = static_cast<uint32_t>(seq_region.location),
      .rank_offset = seq_region.start,
      .size = seq_region.size,
  };

  if (ioctl(device_->get_resource_fd(), DEALLOCATE_MEMORY, &request) < 0) {
    pnm::error::throw_from_errno(errno, pnm::error::ioctl_from_errno);
  }
}

pnm::memory::SequentialRegion
pnm::memory::RankedAllocator::allocate_ioctl(uint64_t size,
                                             std::optional<uint32_t> rank) {
  sls_memory_alloc_request request{
      .rank = rank.has_value() ? *rank : SLS_ALLOC_ANY_RANK,
      .rank_offset = 0,
      .size = size,
  };

  if (ioctl(device_->get_resource_fd(), ALLOCATE_MEMORY, &request) < 0) {
    pnm::error::throw_from_errno(errno);
  }

  return SequentialRegion{.start = request.rank_offset,
                          .size = size,
                          .location = static_cast<int32_t>(request.rank)};
}
