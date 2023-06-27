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

#ifndef _SLS_TEST_GOLDEN_H
#define _SLS_TEST_GOLDEN_H

#include "constants.h"

#include <fmt/core.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace test_app {

template <typename T>
inline std::vector<T>
generate_golden_vec(const std::vector<uint32_t> &lS_indices,
                    const std::vector<uint32_t> &lS_lengths,
                    const std::vector<uint8_t> &tables, size_t num_tables) {
  // Fill golden buffer with -1.0f value for easier error detection.
  std::vector<T> golden_vec(num_tables * kmini_batch_size, -1);

  auto *typed_tables = reinterpret_cast<const T *>(tables.data());
  T cur_golden_val = 0;
  int index_idx = 0;
  for (size_t table_id = 0; table_id < num_tables; ++table_id) {
    const auto table_batch_offset = table_id * kmini_batch_size;
    for (int batch_id = 0; batch_id < kmini_batch_size; ++batch_id) {
      const auto curent_batch_offset = table_batch_offset + batch_id;
      for (auto lookup_count = 0U;
           lookup_count < lS_lengths[curent_batch_offset]; ++lookup_count) {
        const int idx = lS_indices[index_idx++];
        cur_golden_val += *(typed_tables + ((table_id * kemb_table_len + idx) *
                                            ksparse_feature_size));
      }
      golden_vec[curent_batch_offset] = cur_golden_val;
      cur_golden_val = 0.0F;
    }
  }
  return golden_vec;
}

template <typename T>
inline double
perform_golden_check(const std::vector<T> &psum_res,
                     const std::vector<T> &golden_vec,
                     int sparse_feature_size = ksparse_feature_size) {
  assert(sparse_feature_size >= 1);
  auto bad_ident = [=, index = 0](T v) mutable {
    return std::fabs(v - golden_vec[index++ / sparse_feature_size]) >
           kthreshold_val<T>;
  };
  const size_t total = psum_res.size();
  const size_t bad = std::count_if(psum_res.begin(), psum_res.end(), bad_ident);

  const double mismatch_ratio = static_cast<double>(bad) / total * 100;

  fmt::print(
      "Test completed, error ratio: {}% ({}/{}) with threshold value {}.\n",
      mismatch_ratio, bad, total, kthreshold_val<T>);

  return mismatch_ratio;
}

template <typename T>
inline double perform_golden_check(const std::vector<T> &psum_res,
                                   const std::vector<uint32_t> &lS_indices,
                                   const std::vector<uint32_t> &lS_lengths,
                                   const std::vector<uint8_t> &tables,
                                   size_t num_tables) {
  // Calculating the result beforehand
  const auto golden_vec =
      generate_golden_vec<T>(lS_indices, lS_lengths, tables, num_tables);

  return perform_golden_check<T>(psum_res, golden_vec);
}

} // namespace test_app

#endif // _SLS_TEST_GOLDEN_H
