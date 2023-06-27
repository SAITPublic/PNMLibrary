
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

#include "transfer_manager.h"

#include "core/memory/barrier.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/views.h"

#include <algorithm>
#include <cstdint>
#include <iterator>

uint64_t pnm::memory::IMDBTransferManager::copy_from_device_impl(
    const DeviceRegionInfo &dev_region, pnm::common_view<uint8_t> host_region) {
  const auto &dev_vregion = std::get<VRegionType>(dev_region.virt);

  if (host_region.size() < dev_vregion.size()) {
    throw pnm::error::InvalidArguments(
        "The destination region is too small to store all data.");
  }

  // Here we use simple std::copy. In POC, the avx-base copy are used. At the
  // first time, we should try trivial approach, but keep in mind that
  // we will need it (to avoid cache and add l/s-fences) sometime.
  // [TODO: y-lavrinenko] Use AVX-memcpy from sandbox when the real device will
  // be used
  if constexpr (PNM_PLATFORM != FUNCSIM) {
    flush(dev_vregion.begin(), dev_vregion.size());
  }
  auto *end_it =
      std::copy(dev_vregion.begin(), dev_vregion.end(), host_region.begin());

  return std::distance(host_region.begin(), end_it);
}

uint64_t pnm::memory::IMDBTransferManager::copy_to_device_impl(
    pnm::common_view<const uint8_t> host_region,
    const DeviceRegionInfo &dev_region) {
  auto dev_vregion = std::get<VRegionType>(dev_region.virt);

  if (dev_vregion.size() < host_region.size()) {
    throw pnm::error::InvalidArguments(
        "Device region should have size equal or greater than host region");
  }

  // Here we use simple std::copy. In POC, the avx-base copy are used. At the
  // first time, we should try trivial approach, but keep in mind that
  // we will need it (to avoid cache and add l/s-fences) sometime.
  // [TODO: y-lavrinenko] Use AVX-memcpy from sandbox when the real device will
  // be used
  auto *end_it =
      std::copy(host_region.begin(), host_region.end(), dev_vregion.begin());
  if constexpr (PNM_PLATFORM != FUNCSIM) {
    flush(dev_vregion.begin(), dev_vregion.size());
  }

  return std::distance(dev_vregion.begin(), end_it);
}
