
/*
 *  Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 *  This software is proprietary of Samsung Electronics.
 *  No part of this software, either material or conceptual may be copied or
 *  distributed, transmitted, transcribed, stored in a retrieval system or
 *  translated into any human or computer language in any form by any means,
 *  electronic, mechanical, manual or otherwise, or disclosed to third parties
 *  without the express written permission of Samsung Electronics.
 */

#ifndef PNM_HW_RUNNER_H
#define PNM_HW_RUNNER_H

#include "imdb/utils/runner/iscan.h"

#include "pnmlib/imdb/scan.h"
#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/core/buffer.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/runner.h"

#include "pnmlib/common/views.h"

#include <type_traits>
#include <utility>

using pnm::operations::Scan;

namespace test_app {

using namespace pnm::imdb;
using namespace pnm::imdb::runner;
class ImdbRunner : public IRunner {
public:
  ImdbRunner(compressed_vector column, pnm::ContextHandler &context)
      : IRunner(std::move(column)), context_(context),
        column_buf_(pnm::views::make_view(column_.container()), context_) {}

private:
  template <typename Output> auto make_result_buffer(Output &result) const {
    if constexpr (std::is_same_v<Output, index_vector>) {
      return pnm::memory::Buffer<index_type>(pnm::views::make_view(result),
                                             context_);
    } else {
      return pnm::memory::Buffer<index_type>(
          pnm::views::make_view(result.container()), context_);
    }
  }

  template <typename Predicate>
  auto invoke_runner(pnm::Runner &runner, Predicate &predicate,
                     pnm::memory::Buffer<index_type> &result_buffer,
                     OutputType output_type) const {
    if constexpr (std::is_same_v<Predicate, RangeOperation>) {
      pnm::operations::Scan op(
          Scan::Column{column_.value_bits(), column_.size(), &column_buf_},
          predicate, &result_buffer, output_type);

      runner.run(op);

      return op.result_size();
    } else {
      const pnm::memory::Buffer predicate_buffer(
          pnm::views::make_view(predicate.container()), context_);

      pnm::operations::Scan op(
          Scan::Column{column_.value_bits(), column_.size(), &column_buf_},
          Scan::InListPredicate{predicate.size(), &predicate_buffer},
          &result_buffer, output_type);

      runner.run(op);

      return op.result_size();
    }
  }

  template <typename T>
  static constexpr auto operation_type_v =
      std::is_same_v<std::decay_t<T>, index_vector> ? OutputType::IndexVector
                                                    : OutputType::BitVector;

  template <typename Output, typename Predicates>
  Output scan_impl(const Predicates &predicates) const {
    pnm::Runner runner(context_);

    using output_etype = typename Output::value_type;
    Output out;

    std::transform(predicates.begin(), predicates.end(),
                   std::back_inserter(out), [this, &runner](auto predicate) {
                     output_etype result(column_.size());

                     auto result_buffer = make_result_buffer(result);

                     [[maybe_unused]] auto matched =
                         invoke_runner(runner, predicate, result_buffer,
                                       operation_type_v<output_etype>);

                     result_buffer.copy_from_device();

                     if constexpr (std::is_same_v<output_etype, index_vector>) {
                       result.resize(matched);
                     }

                     return result;
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

  pnm::ContextHandler &context_;
  pnm::memory::Buffer<compressed_element_type> column_buf_;
};
} // namespace test_app

#endif // PNM_HW_RUNNER_H
