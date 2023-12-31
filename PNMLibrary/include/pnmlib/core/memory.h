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

#ifndef _PNM_MEMORY_H_
#define _PNM_MEMORY_H_

#include "pnmlib/common/views.h"

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace pnm::memory {
/** @brief The sequential device's memory region */
struct SequentialRegion {
  uintptr_t start = 0; // region start address
  uint64_t size = 0;   // region size
  std::optional<uint8_t> location =
      std::nullopt;      // location tag (rank id or similar)
  uint8_t is_global = 0; // is region in global pool
};

// [TODO: MCS23-432] Use default comparison operator
inline bool operator==(const SequentialRegion &lhs,
                       const SequentialRegion &rhs) {
  return lhs.start == rhs.start && lhs.size == rhs.size &&
         lhs.location == rhs.location && lhs.is_global == rhs.is_global;
}

// [TODO @e-kutovoi MCS23-1260]: Make this a `using` declaration

// Base object to describe data's location in the device memory
struct RankedRegion {
  std::vector<SequentialRegion> regions;
};

inline bool operator==(const RankedRegion &lhs, const RankedRegion &rhs) {
  return lhs.regions == rhs.regions;
}

using DeviceRegion = std::variant<SequentialRegion, RankedRegion>;

using VirtualSequentialRegion = pnm::views::common<uint8_t>;
using VirtualRankedRegion = std::vector<pnm::views::common<uint8_t>>;
using VirtualRegion =
    std::variant<VirtualSequentialRegion, VirtualRankedRegion>;

} // namespace pnm::memory

#endif //_PNM_MEMORY_H_
