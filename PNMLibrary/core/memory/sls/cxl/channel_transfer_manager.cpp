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

#include "channel_transfer_manager.h"

#include "common/memory/barrier.h"

#include "pnmlib/core/transfer_manager.h"

#include "pnmlib/common/views.h"

#include <algorithm>
#include <cstdint>
#include <iterator>

uint64_t pnm::memory::ChannelTransferManager::copy_to_device_impl(
    pnm::views::common<const uint8_t> host_region,
    const pnm::memory::TransferManager::DeviceRegionInfo &dev_region) {
  const auto &vregions = std::get<VRegionType>(dev_region.virt);

  uint64_t copied = 0;
  for (auto vchannel_region : vregions) {
    if (!vchannel_region.empty()) {
      auto *it = std::copy(host_region.begin(), host_region.end(),
                           vchannel_region.begin());
      pnm::memory::mfence();
      copied = std::distance(vchannel_region.begin(), it);
    }
  }
  return copied;
}

uint64_t pnm::memory::ChannelTransferManager::copy_from_device_impl(
    const pnm::memory::TransferManager::DeviceRegionInfo &dev_region,
    pnm::views::common<uint8_t> host_region) {
  const auto &vregions = std::get<VRegionType>(dev_region.virt);

  uint64_t copied = 0;
  for (auto vchannel_region : vregions) {
    if (!vchannel_region.empty()) {
      auto *it = std::copy(vchannel_region.begin(), vchannel_region.end(),
                           host_region.begin());
      pnm::memory::mfence();
      copied = std::distance(host_region.begin(), it);

      // Here we assume that all regions contain same data
      // The assumption is based on statement that in replicate strategy we have
      // the same data in all regions
      break;
    }
  }

  return copied;
}
