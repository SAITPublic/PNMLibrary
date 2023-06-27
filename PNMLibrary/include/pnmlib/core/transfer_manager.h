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

#ifndef _PNM_TRANSFER_MANAGER_H_
#define _PNM_TRANSFER_MANAGER_H_

#include "pnmlib/core/accessor.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/views.h"

#include <cassert>
#include <cstdint>

namespace pnm::memory {

class TransferManager {
public:
  template <typename T>
  uint64_t copy_to_device(common_view<T> host_region, Accessor<T> accessor) {
    auto casted_range = view_cast<const uint8_t>(host_region);
    const auto copied = copy_to_device_impl(
        casted_range,
        DeviceRegionInfo{accessor.virtual_range(), accessor.phys_range()});
    assert(copied == static_cast<uint64_t>(casted_range.size()) &&
           "Not all elements were copied");

    return copied / sizeof(T);
  }

  template <typename T>
  uint64_t copy_from_device(Accessor<T> accessor, common_view<T> host_region) {
    auto casted_range = view_cast<uint8_t>(host_region);
    const auto copied = copy_from_device_impl(
        DeviceRegionInfo{accessor.virtual_range(), accessor.phys_range()},
        casted_range);
    assert(copied == accessor.size() * sizeof(T) &&
           "Not all elements were copied");

    return copied / sizeof(T);
  }

  virtual ~TransferManager() = default;

protected:
  struct DeviceRegionInfo {
    VirtualRegion virt;
    DeviceRegion phys;
  };

private:
  virtual uint64_t copy_to_device_impl(common_view<const uint8_t> host_region,
                                       const DeviceRegionInfo &dev_region) = 0;
  virtual uint64_t copy_from_device_impl(const DeviceRegionInfo &dev_region,
                                         common_view<uint8_t> host_region) = 0;
};

} // namespace pnm::memory

#endif //_PNM_TRANSFER_MANAGER_H_
