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

#ifndef CPU_SCAN_RUNNER_H
#define CPU_SCAN_RUNNER_H

#include "iscan.h"

#include "imdb/software_scan/nosimd/cpu_scan.h"

#include "pnmlib/imdb/scan_types.h"

#include <algorithm>
#include <iterator>
#include <vector>

namespace pnm::imdb::runner {

class CPURunner : public IRunner {
public:
  using IRunner::IRunner;

private:
  template <typename Output, typename Input, typename Function>
  auto dispatch_compute_operation(const Input &input, Function function) const {
    std::vector<Output> result;
    result.reserve(input.size());

    std::transform(input.begin(), input.end(), std::back_inserter(result),
                   [this, function](const auto &in) {
                     Output ret;
                     function(column_, in, ret);
                     return ret;
                   });

    return result;
  }

  BitVectors compute_in_range_to_bv_impl(const Ranges &ranges) const override {
    return dispatch_compute_operation<bit_vector>(
        ranges, internal::cpu::in_range_to_bv);
  }

  IndexVectors
  compute_in_range_to_iv_impl(const Ranges &ranges) const override {
    return dispatch_compute_operation<index_vector>(
        ranges, internal::cpu::in_range_to_iv);
  }

  BitVectors
  compute_in_list_to_bv_impl(const Predictors &predictors) const override {
    return dispatch_compute_operation<bit_vector>(predictors,
                                                  internal::cpu::in_list_to_bv);
  }

  IndexVectors
  compute_in_list_to_iv_impl(const Predictors &predictors) const override {
    return dispatch_compute_operation<index_vector>(
        predictors, internal::cpu::in_list_to_iv);
  }
};

} // namespace pnm::imdb::runner

#endif // CPU_SCAN_RUNNER_H
