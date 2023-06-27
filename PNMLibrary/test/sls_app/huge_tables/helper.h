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

#ifndef _SLS_TEST_HUGE_TABLES_H
#define _SLS_TEST_HUGE_TABLES_H

#include "test/sls_app/api/constants.h"
#include "test/sls_app/api/golden.h"
#include "test/sls_app/api/helper.h"
#include "test/sls_app/api/structs.h"

#include "pnmlib/sls/embedded_tables.h"
#include "pnmlib/sls/sls.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <linux/sls_resources.h>

#include <cstdint>
#include <vector>

namespace test_app {

template <typename T> class SLSTestHugeTables : private SLSTestHelper<T> {
public:
  using SLSTestHelper<T>::SLSTestHelper;

  ~SLSTestHugeTables() override = default;

  /**
      @brief
      1. Ensure that we can't allocate it by SLS_ALLOC_REPLICATE_ALL.
      2. Allocate by SLS_ALLOC_DISTRIBUTE_ALL and write rank data.
      3. Run sls operation and perform golden check.
  */
  void run(const std::vector<uint8_t> &tables,
           const SlsOpParams &sls_op_params) const;

  void print() const {
    fmt::print("SLSTestHugeTables\n");
    this->print_params();
  }
};

template <typename T>
void SLSTestHugeTables<T>::run(const std::vector<uint8_t> &tables,
                               const SlsOpParams &sls_op_params) const {
  sls_start_profiling();

  pnm::memory::EmbeddedTablesHandler embedded_tables;

  EXPECT_THROW(embedded_tables = pnm::memory::EmbeddedTables::create(
                   pnm::make_view(tables), this->model_params_.tables_rows_num,
                   this->model_params_.template feature_size_in_bytes<T>(),
                   this->ctx_, SLS_ALLOC_REPLICATE_ALL);
               , pnm::error::OutOfMemory);

  fmt::print("\n"
             "SLS_ALLOC_REPLICATE_ALL memory policy fails for huge tables.\n"
             "It's ok, let's move on to SLS_ALLOC_DISTRIBUTE_ALL\n");

  embedded_tables = pnm::memory::EmbeddedTables::create(
      pnm::make_view(tables), this->model_params_.tables_rows_num,
      this->model_params_.template feature_size_in_bytes<T>(), this->ctx_,
      SLS_ALLOC_DISTRIBUTE_ALL);

  auto sls_op = this->create_sls_op(embedded_tables.get());
  auto psum_res = this->run_sls(sls_op, sls_op_params, this->runner_);

  const double error_ratio = perform_golden_check(
      psum_res, sls_op_params.lS_indices, sls_op_params.lS_lengths, tables,
      this->model_params_.num_tables());
  EXPECT_LE(error_ratio, kmismatch_ratio_threshold_val);

  sls_report_profile();
}
} // namespace test_app

#endif // _SLS_TEST_HUGE_TABLES_H
