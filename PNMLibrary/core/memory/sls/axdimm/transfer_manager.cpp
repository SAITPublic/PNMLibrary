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

#include "transfer_manager.h"

#include "core/device/sls/rank_address.h"

#include "pnmlib/core/transfer_manager.h"

#include "pnmlib/common/views.h"

#include <cstdint>

uint64_t pnm::memory::AXDIMMTransferManager::copy_to_device_impl(
    pnm::common_view<const uint8_t> host_region,
    const pnm::memory::TransferManager::DeviceRegionInfo &dev_region) {
  const auto &vregion = std::get<VRegionType>(dev_region.virt);
  const auto &pregion = std::get<RegionType>(dev_region.phys);

  uint64_t copied = 0;
  for (auto i = 0UL; i < vregion.size(); ++i) {
    if (!vregion[i].empty()) {
      auto ipointer = pnm::sls::device::make_interleaved_pointer(
          pregion.regions[i].location, pregion.regions[i].start,
          vregion[i].begin());

      copied = pnm::sls::device::memcpy_interleaved(
          host_region.begin(), host_region.size(), ipointer);
    }
  }
  return copied;
}

uint64_t pnm::memory::AXDIMMTransferManager::copy_from_device_impl(
    const pnm::memory::TransferManager::DeviceRegionInfo &dev_region,
    pnm::common_view<uint8_t> host_region) {
  const auto &vregion = std::get<VRegionType>(dev_region.virt);
  const auto &pregion = std::get<RegionType>(dev_region.phys);

  uint64_t copied = 0;
  for (auto i = 0UL; i < vregion.size(); ++i) {
    if (!vregion[i].empty()) {
      auto ipointer = pnm::sls::device::make_interleaved_pointer(
          pregion.regions[i].location, pregion.regions[i].start,
          vregion[i].begin());

      copied = pnm::sls::device::memcpy_interleaved(
          ipointer, host_region.size(), host_region.begin());

      // Assume that all ranks contains same data according to replicate policy
      break;
    }
  }
  return copied;
}
