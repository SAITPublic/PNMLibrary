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

#ifndef _IMDB_TRANSFER_MANAGER_H_
#define _IMDB_TRANSFER_MANAGER_H_

#include "pnmlib/core/memory.h"
#include "pnmlib/core/transfer_manager.h"

#include "pnmlib/common/views.h"

#include <cstdint>

namespace pnm::memory {
class IMDBTransferManager : public TransferManager {
private:
  using VRegionType = VirtualSequentialRegion;

  uint64_t copy_to_device_impl(common_view<const uint8_t> host_region,
                               const DeviceRegionInfo &dev_region) override;

  uint64_t copy_from_device_impl(const DeviceRegionInfo &dev_region,
                                 common_view<uint8_t> host_region) override;
};
} // namespace pnm::memory

#endif //_IMDB_TRANSFER_MANAGER_H_
