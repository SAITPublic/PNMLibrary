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

#ifndef _SEQUENTIAL_ACCESSOR_H_
#define _SEQUENTIAL_ACCESSOR_H_

#include "common/mapped_file.h"

#include "pnmlib/core/accessor.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include <unistd.h>

#include <cassert>
#include <cstdint>

namespace pnm::memory {
/**@brief The implementation of accessor logic for device with sequential memory
 * layout.
 */
class SequentialAccessorCore : public AccessorCore {
public:
  SequentialAccessorCore(const DeviceRegion &region, DevicePointer &device,
                         uint64_t address_align = page_size());

private:
  static long page_size() {
    auto page_size = sysconf(_SC_PAGESIZE);
    assert(page_size >= 1 && "Invalid page size");
    return page_size;
  }

  const uint8_t *access_impl(uint64_t byte_offset,
                             uint64_t size) const override;

  VirtualRegion virtual_range_impl() const override;

  uint64_t bytes_impl() const override;

  void store_impl(const void *value, uint64_t size,
                  uint64_t byte_offset) override;

  DeviceRegion phys_range_impl() const override;

  SequentialRegion region_;

  uint64_t address_align_;

  pnm::utils::MappedData device_map_;

  VirtualSequentialRegion virtual_region_;
};
} // namespace pnm::memory

#endif //_SEQUENTIAL_ACCESSOR_H_
