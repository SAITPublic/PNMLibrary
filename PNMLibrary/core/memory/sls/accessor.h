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

#ifndef PNM_SLS_ACCESSOR_H
#define PNM_SLS_ACCESSOR_H

#include "common/mapped_file.h"

#include "pnmlib/core/accessor.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include <cstdint>
#include <vector>

namespace pnm::memory {
class SLSAccessorCore : public AccessorCore {
public:
  using RegionType = RankedRegion;

  SLSAccessorCore(const DeviceRegion &region, DevicePointer &device);

private:
  void store_impl(const void *value, uint64_t size,
                  uint64_t byte_offset) override;

  DeviceRegion phys_range_impl() const override;

  const uint8_t *access_impl(uint64_t byte_offset) const override;

  VirtualRegion virtual_range_impl() const override;

  uint64_t bytes_impl() const override;

  void init_virtual_region();

  int device_id_;
  RankedRegion region_;
  uint64_t data_size_ = 0; // in bytes
  std::vector<pnm::utils::MappedData> vregion_data_;
  VirtualRankedRegion vregion_;
};
} // namespace pnm::memory

#endif // PNM_SLS_ACCESSOR_H
