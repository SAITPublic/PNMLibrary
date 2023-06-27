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

#ifndef _AVX2_SCAN_RUNNER_H_
#define _AVX2_SCAN_RUNNER_H_

#include "iscan.h"

#include "imdb/software_scan/avx2/scan.h"

#include "pnmlib/imdb/scan_types.h"

#include <utility>

namespace pnm::imdb::runner {

class AVX2Runner : public IRunner {
public:
  explicit AVX2Runner(pnm::imdb::compressed_vector cv)
      : IRunner(std::move(cv)) {}

private:
  template <typename Output, typename Predicate>
  Output scan_impl(const Predicate &predicates) const {
    using output_etype = typename Output::value_type;
    Output out;

    auto column_view = this->column_.view();

    auto make_output_view = [](auto &output) {
      if constexpr (std::is_same_v<output_etype, pnm::imdb::index_vector>) {
        return pnm::make_view(output);
      } else {
        return output.view();
      }
    };

    auto forward_request = [](const auto &req) {
      if constexpr (std::is_same_v<decltype(req),
                                   const pnm::imdb::RangeOperation &>) {
        return req;
      } else {
        return req.view();
      }
    };

    std::transform(
        predicates.begin(), predicates.end(), std::back_inserter(out),
        [this, column_view, make_output_view, forward_request](auto range) {
          output_etype out_element(column_.size());
          auto output_view = make_output_view(out_element);
          [[maybe_unused]] auto size = pnm::imdb::internal::avx2::scan(
              column_view, forward_request(range), output_view);
          if constexpr (std::is_same_v<output_etype, pnm::imdb::index_vector>) {
            out_element.resize(size);
          }

          return out_element;
        });

    return out;
  }

  BitVectors compute_in_range_to_bv_impl(const Ranges &ranges) const override {
    return scan_impl<BitVectors>(ranges);
  }

  IndexVectors
  compute_in_range_to_iv_impl(const Ranges &ranges) const override {
    return scan_impl<IndexVectors>(ranges);
  }

  BitVectors
  compute_in_list_to_bv_impl(const Predictors &predictors) const override {
    return scan_impl<BitVectors>(predictors);
  }

  IndexVectors
  compute_in_list_to_iv_impl(const Predictors &predictors) const override {
    return scan_impl<IndexVectors>(predictors);
  }
};

} // namespace pnm::imdb::runner

#endif //_AVX2_SCAN_RUNNER_H_
