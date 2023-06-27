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

#ifndef SLS_TRAITS_H
#define SLS_TRAITS_H

#include <cstdint>

template <typename T> struct DataTypeTraits;

template <> struct DataTypeTraits<float> {
  constexpr static const char *name = "float";
  constexpr static const float seed_sparse_feature_val = 0.01F;
};

template <> struct DataTypeTraits<uint32_t> {
  constexpr static const char *name = "uint32_t";
  constexpr static const uint32_t seed_sparse_feature_val = 1;
};

#endif // SLS_TRAITS_H
