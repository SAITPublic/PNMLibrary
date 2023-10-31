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

#ifndef PNM_CHANNEL_TRANSFER_MANAGER_H
#define PNM_CHANNEL_TRANSFER_MANAGER_H

#include "pnmlib/core/memory.h"
#include "pnmlib/core/transfer_manager.h"

#include "pnmlib/common/views.h"

#include <cstdint>

namespace pnm::memory {

class ChannelTransferManager : public TransferManager {
private:
  using VRegionType = VirtualRankedRegion;
  using RegionType = RankedRegion;
  uint64_t copy_to_device_impl(pnm::views::common<const uint8_t> host_region,
                               const DeviceRegionInfo &dev_region) override;
  uint64_t
  copy_from_device_impl(const DeviceRegionInfo &dev_region,
                        pnm::views::common<uint8_t> host_region) override;
};

} // namespace pnm::memory

#endif // PNM_CHANNEL_TRANSFER_MANAGER_H
