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

// IWYU pragma: private: include "pnm_addr_constants.h"

#ifndef PNM_PNM_SIM_ADDR_CONSTANTS_H
#define PNM_PNM_SIM_ADDR_CONSTANTS_H

#include "hw/pnm_addr_constants_types.h"

#include <array>

namespace pnm::hw { // NOLINT(modernize-concat-nested-namespaces)

inline namespace hw_sim_addr_map {

inline constexpr std::array COL = {
    BitRegionEntry{.RAOffset = 3, .Mask = 0x1F, .StartBit = 0},
    BitRegionEntry{.RAOffset = 14, .Mask = 0x1F, .StartBit = 5},
};

inline constexpr std::array ROW = {
    BitRegionEntry{.RAOffset = 8, .Mask = 0x0F, .StartBit = 0},
    BitRegionEntry{.RAOffset = 28, .Mask = 0x01, .StartBit = 4},
    BitRegionEntry{.RAOffset = 21, .Mask = 0x07F, .StartBit = 5},
    BitRegionEntry{.RAOffset = 29, .Mask = 0x01F, .StartBit = 12},
};

inline constexpr std::array BG = {
    BitXorEntry{.RAOffset0 = 21, .RAOffset1 = 12, .StartBit = 0},
    BitXorEntry{.RAOffset0 = 22, .RAOffset1 = 13, .StartBit = 1},
};

inline constexpr std::array BANK = {
    BitXorEntry{.RAOffset0 = 23, .RAOffset1 = 19, .StartBit = 0},
    BitXorEntry{.RAOffset0 = 24, .RAOffset1 = 20, .StartBit = 1},
};

// interleaving constants
inline constexpr auto HA_XOR_BIT = 19; // Home Agent XOR bit position
inline constexpr auto HA_BIT_POS = 8;  // Home Agent bit position
inline constexpr auto HA_MASK = 0b1;   // Home Agent mask
} // namespace hw_sim_addr_map
} // namespace pnm::hw

#endif // PNM_PNM_SIM_ADDR_CONSTANTS_H
