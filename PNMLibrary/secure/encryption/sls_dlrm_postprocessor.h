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
#ifndef SLS_DLRM_POSTPROCESSOR_H
#define SLS_DLRM_POSTPROCESSOR_H

#include "sls_postrocessor.h"

#include "secure/encryption/sls_engine.h"
#include "secure/encryption/sls_mac.h"

#include "common/compiler_internal.h"

#include "pnmlib/common/128bit_math.h"
#include "pnmlib/common/views.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <numeric>
#include <utility>
#include <vector>

namespace pnm::sls::secure {

/*! \brief Class that handle information about tables that was involved in
 * encryption process. The main aim of this class is perform decryption and
 * validation.
 * @tparam T -- is a type of table elements
 * */
template <typename T> class DLRMPostprocessor : public IPostprocessor<T> {
public:
  using EEngine = pnm::sls::secure::EncryptionEngine<T>;
  using VEngine = pnm::sls::secure::VerificationEngine<>;

  DLRMPostprocessor(EEngine e_engine, VEngine v_engine,
                    std::vector<uintptr_t> table_offsets, uintptr_t cols_count)
      : e_engine_{std::move(e_engine)}, v_engine_{std::move(v_engine)},
        offsets_(std::move(table_offsets)), cols_count_{cols_count} {}

private:
  void fill_offsets(size_t table_id, pnm::views::common<const uint32_t> indices,
                    std::vector<uintptr_t> &selected_offsets) const {
    auto table_offset = offsets_[table_id];
    selected_offsets.resize(indices.size());
    std::transform(indices.begin(), indices.end(), selected_offsets.begin(),
                   [table_offset, this](auto index) {
                     return table_offset + (index * cols_count_) * sizeof(T);
                   });
  }

  void perform_sls_impl(pnm::views::common<const requests_view> indices,
                        psums_view<T> psum, tags_view<T> tags) const override {
    // SLS for TAG
    auto sls_op =
        [](auto sum, const auto &value)
            NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW { return sum + value; };

    // SLS for psum
    auto sls_op_psum = [](auto sum, const auto &value) {
      auto plus = [](const auto &l, const auto &r)
                      NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW { return l + r; };

      std::transform(value.begin(), value.end(), sum.begin(), sum.begin(),
                     plus);
      return sum;
    };

    long psum_index = 0;
    std::vector<uintptr_t> sls_offset; // Buffer to avoid multiple allocation

    for (const auto &[table_id, batch] : indices) {
      for (const auto &lookup : batch) {
        fill_offsets(table_id, lookup, sls_offset);

        auto current_psum = *(psum.begin() + psum_index);
        std::fill(current_psum.begin(), current_psum.end(), T{});

        auto offset_view = pnm::views::make_view(sls_offset);
        e_engine_.offset_transform_reduce_vec(
            offset_view.begin(), offset_view.end(), current_psum, sls_op_psum);

        if (tags.size()) {
          auto etag = v_engine_.offset_transform_reduce_vec(
              offset_view.begin(), offset_view.end(), pnm::types::uint128_t{},
              sls_op);
          *reinterpret_cast<pnm::types::uint128_t *>(
              (tags.begin() + psum_index)->begin()) = etag;
        }

        ++psum_index;
      }
    }
  }

  bool
  decrypt_psum_impl(psums_view<T> psums, psums_view<T> epsums,
                    tags_view<T> ctags, tags_view<T> etags,
                    pnm::views::common<uint8_t> check_result) const override {
    auto ctag_it = ctags.begin();
    auto etag_it = etags.begin();
    auto *check_it = check_result.begin();

    auto func = [&](auto &cres, const auto &eres) -> bool {
      e_engine_.decrypt_psum(cres.begin(), cres.end(), eres.begin(),
                             cres.begin());
      if (ctags.size() && etags.size()) {
        auto ET = *reinterpret_cast<pnm::types::uint128_t *>(ctag_it->begin());
        auto CT = *reinterpret_cast<pnm::types::uint128_t *>(etag_it->begin());
        *check_it = v_engine_.validate_psum(cres.begin(), cres.end(), ET, CT);
        ++ctag_it, ++etag_it;
        return *(check_it++);
      }
      return true;
    };

    return std::inner_product(psums.begin(), psums.end(), epsums.begin(), true,
                              std::logical_and{}, func);
  }

  EEngine e_engine_;
  VEngine v_engine_;
  std::vector<uintptr_t> offsets_;
  uint64_t cols_count_;
};
} // namespace pnm::sls::secure

#endif // SLS_DLRM_POSTPROCESSOR_H
