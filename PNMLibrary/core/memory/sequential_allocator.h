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

#ifndef _SEQUENTIAL_ALLOCATOR_H_
#define _SEQUENTIAL_ALLOCATOR_H_

#include "pnmlib/core/allocator.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/properties.h"

#include <cstdint>

namespace pnm::memory {

// [TODO: MCS23-1130] Finalize untying after kernel changes
/** @brief Sequential memory allocator.
 *
 * The class implements sequential memory allocation logic.
 *
 * */
class SequentialAllocator : public Allocator {
public:
  using RegionType = SequentialRegion;

  explicit SequentialAllocator(const Device *dev) : device_{dev} {}

private:
  DeviceRegion
  allocate_impl(uint64_t size,
                const pnm::property::PropertiesList &props) override;

  void deallocate_impl(const DeviceRegion &region) override;
  uint8_t
  get_alloc_preference(const pnm::property::PropertiesList &props) const;

  const Device *device_;
};

} // namespace pnm::memory

#endif //_SEQUENTIAL_ALLOCATOR_H_
