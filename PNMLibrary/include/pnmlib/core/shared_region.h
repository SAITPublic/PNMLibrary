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

#ifndef _PNM_SHARED_REGION_H_
#define _PNM_SHARED_REGION_H_

#include "pnmlib/core/memory.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace pnm::memory {
/** @brief Represents a region, which can be shared between independent
 * processes*/
class SharedRegion {
public:
  using SerializedRegion = std::vector<uint64_t>;

  explicit SharedRegion(const DeviceRegion &region);

  explicit SharedRegion(const SerializedRegion &serialized);

  SerializedRegion get_serialized() const { return serialized_region_; }

  DeviceRegion get_region() const { return region_; }

private:
  uint64_t serialize_sequential(const SequentialRegion &region);

  SequentialRegion deserialize_sequential(uint64_t serialized_region,
                                          size_t amount_of_regions);

  void serialize();

  void deserialize(const SerializedRegion &serialized);

  DeviceRegion region_;
  SerializedRegion serialized_region_;

  // Poisition where we hold cunit in serialized region(56:63)
  static constexpr size_t cunit_pos_in_serialized_ = 56;
  // We can hold up to 256 cunits, because memory_pool is uint8_t
  static constexpr size_t max_amount_of_regions_ = 256;
  static constexpr size_t max_index_of_region_ = 255;
};
} // namespace pnm::memory

#endif // _PNM_SHARED_REGION_H_
