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

#ifndef PNM_PNM_ADDR_CONSTANTS_TYPES_H
#define PNM_PNM_ADDR_CONSTANTS_TYPES_H

#include <cstdint>

namespace pnm::hw {
struct BitRegionEntry {
  uint64_t RAOffset; // RA entry start offset
  uint64_t Mask;     // Entry mask
  uint64_t StartBit; // Entry bit position
};

struct BitXorEntry {
  uint64_t RAOffset0; // RA offset xor bit position
  uint64_t RAOffset1; // RA offset bit position
  uint64_t StartBit;  // Entry bit position
};
} // namespace pnm::hw

#endif // PNM_PNM_ADDR_CONSTANTS_TYPES_H
