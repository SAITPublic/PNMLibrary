/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#ifndef PNM_128BIT_MATH_H
#define PNM_128BIT_MATH_H

namespace pnm::types {

template <typename T> struct longint_traits {};

using uint128_t = unsigned __int128;
template <> struct longint_traits<uint128_t> {
  static constexpr auto BIG_VALUE = ((unsigned __int128)(1) << 127) - 1;
};
} // namespace pnm::types

#endif // PNM_128BIT_MATH_H
