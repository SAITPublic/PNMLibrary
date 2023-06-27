/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or
 * computer language in any form
 * by any means, electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung
 * Electronics.
 */

#ifndef CPU_SLS_H
#define CPU_SLS_H

#include "common/compiler_internal.h"

#include "pnmlib/common/128bit_math.h"
#include "pnmlib/common/rowwise_view.h"
#include "pnmlib/common/views.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

template <typename T> struct SLS {
  static void common(pnm::rowwise_view<const T> table,
                     pnm::common_view<const uint32_t> indices,
                     pnm::common_view<T> psum) {
    auto plus = [](const auto &l, const auto &r)
                    NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW { return l + r; };
    std::fill(psum.begin(), psum.end(), T{});
    for (auto index : indices) {
      const auto row = *(table.begin() + index);
      std::transform(psum.begin(), psum.end(), row.begin(), psum.begin(), plus);
    }
  }

  static void
  on_tag(pnm::rowwise_view<const T> tags,
         pnm::common_view<const uint32_t> indices,
         pnm::common_view<T> psum_tag) NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW {

    auto &psum_tag_value =
        *reinterpret_cast<pnm::uint128_t *>(psum_tag.begin());
    psum_tag_value = pnm::uint128_t{};

    for (auto index : indices) {
      const auto tag = *(tags.begin() + index);
      psum_tag_value += *reinterpret_cast<const pnm::uint128_t *>(tag.begin());
    }
  }

  static void compute(const T *table_begin, const std::vector<uint32_t> &rows,
                      const uint32_t *indices_begin, const uint32_t *length,
                      size_t sparse_feature_size, size_t minibatch_size,
                      T *psum) {

    auto psum_size = minibatch_size * rows.size() * sparse_feature_size;
    auto psum_v =
        pnm::make_rowwise_view(psum, psum + psum_size, sparse_feature_size);

    auto cpsum_it = psum_v.begin();
    for (auto rows_count : rows) {
      auto table_v = pnm::make_rowwise_view(
          table_begin, table_begin + sparse_feature_size * rows_count,
          sparse_feature_size);

      for (auto batch_id = 0U; batch_id < minibatch_size; ++batch_id) {
        auto indices_v = pnm::make_view(indices_begin, indices_begin + *length);

        SLS<T>::common(table_v, indices_v, *cpsum_it);

        ++cpsum_it;
        std::advance(indices_begin, *length);
        ++length;
      }

      std::advance(table_begin, rows_count * sparse_feature_size);
    }
  }

  static void
  compute_with_tag(const T *table_begin, const std::vector<uint32_t> &rows,
                   const uint32_t *indices_begin, const uint32_t *length,
                   size_t sparse_feature_size, size_t minibatch_size, T *psum) {

    static constexpr auto single_tag_size = sizeof(pnm::uint128_t) / sizeof(T);

    const auto psum_size = minibatch_size * rows.size() * sparse_feature_size;

    auto sf_psum_v =
        pnm::make_rowwise_view(psum, psum + psum_size, sparse_feature_size);

    const auto tags_meta_size = minibatch_size * rows.size() * single_tag_size;
    auto tag_psum_v = pnm::make_rowwise_view(
        psum + psum_size, psum + psum_size + tags_meta_size, single_tag_size);

    auto cpsum_it = sf_psum_v.begin();
    auto ctag_it = tag_psum_v.begin();

    const auto augmented_row_size = sparse_feature_size + single_tag_size;
    for (auto rows_count : rows) {
      auto table_v = pnm::make_rowwise_view(
          table_begin, table_begin + augmented_row_size * rows_count,
          augmented_row_size, 0, single_tag_size);

      auto tag_v = pnm::make_rowwise_view(
          table_begin, table_begin + augmented_row_size * rows_count,
          augmented_row_size, sparse_feature_size, 0);

      for (auto batch_id = 0U; batch_id < minibatch_size; ++batch_id) {
        auto indices_v = pnm::make_view(indices_begin, indices_begin + *length);

        SLS<T>::common(table_v, indices_v, *cpsum_it);
        SLS<T>::on_tag(tag_v, indices_v, *ctag_it);

        ++cpsum_it, ++ctag_it;
        std::advance(indices_begin, *length);
        ++length;
      }

      std::advance(table_begin, rows_count * augmented_row_size);
    }
  }
};
#endif // CPU_SLS_H
