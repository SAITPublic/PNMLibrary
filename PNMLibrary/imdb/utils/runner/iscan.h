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

#ifndef SCAN_RUNNER_H
#define SCAN_RUNNER_H

#include "pnmlib/imdb/scan_types.h"

#include <utility>

namespace pnm::imdb::runner {

class IRunner {
public:
  explicit IRunner(compressed_vector column) : column_(std::move(column)) {}

  BitVectors compute_in_range_to_bv(const Ranges &ranges) const {
    return compute_in_range_to_bv_impl(ranges);
  }

  IndexVectors compute_in_range_to_iv(const Ranges &ranges) const {
    return compute_in_range_to_iv_impl(ranges);
  }

  BitVectors compute_in_list_to_bv(const Predictors &predictors) const {
    return compute_in_list_to_bv_impl(predictors);
  }

  IndexVectors compute_in_list_to_iv(const Predictors &predictors) const {
    return compute_in_list_to_iv_impl(predictors);
  }

  virtual ~IRunner() = default;

protected:
  virtual BitVectors
  compute_in_range_to_bv_impl(const Ranges &ranges) const = 0;

  virtual IndexVectors
  compute_in_range_to_iv_impl(const Ranges &ranges) const = 0;

  virtual BitVectors
  compute_in_list_to_bv_impl(const Predictors &predictors) const = 0;

  virtual IndexVectors
  compute_in_list_to_iv_impl(const Predictors &predictors) const = 0;

  compressed_vector column_;
};

} // namespace pnm::imdb::runner

#endif // SCAN_RUNNER_H
