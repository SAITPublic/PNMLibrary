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

#include "pnmlib/sls/embedding_tables.h"
#include "pnmlib/sls/sls.h"

#include "pnmlib/core/runner.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <linux/sls_resources.h>

#include <cstdint>
#include <vector>

namespace test_app {

template <typename T> class SlsTestHugeTables : private SlsTestHelper<T> {
public:
  using SlsTestHelper<T>::SlsTestHelper;

  ~SlsTestHugeTables() override = default;

  /**
      @brief
      1. Ensure that we can't allocate it by SLS_ALLOC_REPLICATE_ALL.
      2. Allocate by SLS_ALLOC_DISTRIBUTE_ALL and write rank data.
      3. Run sls operation and perform golden check.
  */
  void run(const std::vector<uint8_t> &tables, const SlsOpParams &sls_op_params,
           bool distribute_policy) const;

  void print() const {
    fmt::print("SlsTestHugeTables\n");
    this->print_params();
  }

private:
  pnm::Runner runner_{this->ctx_};
};

template <typename T>
void SlsTestHugeTables<T>::run(const std::vector<uint8_t> &tables,
                               const SlsOpParams &sls_op_params,
                               bool distribute_policy) const {
  pnm::profile::sls_start_profiling();

  pnm::memory::EmbeddingTablesHandler embedding_tables;

  if (distribute_policy) {
    EXPECT_THROW(embedding_tables = pnm::memory::EmbeddingTables::create(
                     pnm::views::make_view(tables),
                     this->model_params_.tables_rows_num,
                     this->model_params_.template feature_size_in_bytes<T>(),
                     this->ctx_, SLS_ALLOC_REPLICATE_ALL);
                 , pnm::error::OutOfMemory);

    fmt::print("\n"
               "SLS_ALLOC_REPLICATE_ALL memory policy fails for huge tables.\n"
               "It's ok, let's move on to SLS_ALLOC_DISTRIBUTE_ALL\n");

    embedding_tables = pnm::memory::EmbeddingTables::create(
        pnm::views::make_view(tables), this->model_params_.tables_rows_num,
        this->model_params_.template feature_size_in_bytes<T>(), this->ctx_,
        SLS_ALLOC_DISTRIBUTE_ALL);
  } else {
    embedding_tables = pnm::memory::EmbeddingTables::create(
        pnm::views::make_view(tables), this->model_params_.tables_rows_num,
        this->model_params_.template feature_size_in_bytes<T>(), this->ctx_,
        SLS_ALLOC_REPLICATE_ALL);
  }

  auto sls_op = this->create_sls_op(embedding_tables.get());
  auto psum_res = this->run_sls(sls_op, sls_op_params, this->runner_);

  const double error_ratio = perform_golden_check(
      psum_res, sls_op_params.lS_indices, sls_op_params.lS_lengths, tables,
      this->model_params_.num_tables());
  EXPECT_LE(error_ratio, kmismatch_ratio_threshold_val);

  pnm::profile::sls_report_profile();
}
} // namespace test_app

#endif // _SLS_TEST_HUGE_TABLES_H
