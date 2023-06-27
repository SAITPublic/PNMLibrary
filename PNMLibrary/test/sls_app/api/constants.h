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

#ifndef _SLS_TEST_CONSTANTS_H
#define _SLS_TEST_CONSTANTS_H

#include "pnmlib/sls/type.h"

#include <cstdint>
#include <filesystem>

namespace test_app {

inline constexpr int kmax_engines = 200;

inline constexpr int knum_requests = 10;
inline constexpr float kmismatch_ratio_threshold_val =
    PNM_PLATFORM == HARDWARE ? 5 : 0;
inline constexpr int kemb_table_len = 500000;
inline constexpr auto ksparse_feature_size = 16;
inline constexpr auto kmini_batch_size = 16;
inline constexpr auto knum_idx_value = 32;
inline constexpr auto knum_indices_per_lookup_fix = 40;
inline constexpr auto kdata_type = SLSType::Float;

struct PathToTables {
  static inline const std::filesystem::path value{
      std::filesystem::path{PATH_TO_TABLES}};
};

template <typename T> inline constexpr T kseed_sparse_feature_val = T(0.01F);
template <> inline constexpr uint32_t kseed_sparse_feature_val<uint32_t> = 1;
template <typename T> inline constexpr T kpsum_init_val = T(1.0F);
template <> inline constexpr uint32_t kpsum_init_val<uint32_t> = 1;
template <typename T> inline constexpr T kthreshold_val = T(0.001F);
template <> inline constexpr uint32_t kthreshold_val<uint32_t> = 1;
} // namespace test_app

#endif // _SLS_TEST_CONSTANTS_H
