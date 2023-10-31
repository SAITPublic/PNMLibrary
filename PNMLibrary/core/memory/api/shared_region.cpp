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
#include "pnmlib/core/shared_region.h"

#include "pnmlib/core/memory.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/misc_utils.h"

#include <cstddef>
#include <cstdint>
#include <variant>

namespace pnm::memory {
SharedRegion::SharedRegion(const DeviceRegion &region) : region_(region) {
  serialize();
}

SharedRegion::SharedRegion(const SerializedRegion &serialized) {
  if (serialized.empty()) {
    throw pnm::error::InvalidArguments("Invalid size of serialized region. "
                                       "Must have at least one element");
  }
  // "serialized" is a vector of uint64_t values, which represent key
  // information about region(it's address and number of cunit, where
  // it's located)
  deserialize(serialized);
}

uint64_t SharedRegion::serialize_sequential(const SequentialRegion &region) {
  uint64_t serialized_region = region.start;
  serialized_region |=
      (static_cast<uint64_t>(region.location.value_or(max_index_of_region_))
       << cunit_pos_in_serialized_);
  return serialized_region;
}

SequentialRegion
SharedRegion::deserialize_sequential(uint64_t serialized_region,
                                     size_t amount_of_regions) {
  SequentialRegion sequential_region;

  // [TODO: @b.palkin] Make this scheme more simple.
  // During serialization of a region, which doesn't have any allocation,
  // we set "location" field(if it doesn't have value) to 255 as a "marker".
  // During deserialization we check value of this field using conditions
  // below. We set deserialized value to "location" field, if these conditions
  // were satisfied. Otherwise, it will hold std::nullopt.
  const uint64_t tmp_location = serialized_region >> cunit_pos_in_serialized_;
  if ((tmp_location < max_index_of_region_) ||
      (tmp_location == max_index_of_region_ &&
       amount_of_regions == max_amount_of_regions_)) {
    sequential_region.location = tmp_location;
  }

  serialized_region &= ~(tmp_location << cunit_pos_in_serialized_);
  sequential_region.start = serialized_region;

  return sequential_region;
}

void SharedRegion::serialize() {
  std::visit(pnm::utils::visitor::overload{
                 [this](const SequentialRegion &sequential) {
                   serialized_region_.push_back(
                       serialize_sequential(sequential));
                 },
                 [this](const RankedRegion &ranked) {
                   for (const auto &region : ranked.regions) {
                     serialized_region_.push_back(serialize_sequential(region));
                   }
                 }},
             region_);
}

void SharedRegion::deserialize(const SerializedRegion &serialized) {
  serialized_region_ = serialized;

  if (serialized_region_.size() > 1) {
    RankedRegion ranked_region;
    for (const auto &buf : serialized_region_) {
      ranked_region.regions.push_back(
          deserialize_sequential(buf, serialized_region_.size()));
    }
    region_ = ranked_region;
  } else {
    region_ = deserialize_sequential(serialized_region_.front(),
                                     serialized_region_.size());
  }
}
} // namespace pnm::memory
