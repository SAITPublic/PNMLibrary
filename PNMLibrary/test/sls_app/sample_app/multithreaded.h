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

#ifndef _SLS_TEST_MULTITHREADED_H
#define _SLS_TEST_MULTITHREADED_H

#include "standard_helper.h"

#include "test/sls_app/api/constants.h"
#include "test/sls_app/api/structs.h"

#include "pnmlib/sls/embedded_tables.h"
#include "pnmlib/sls/sls.h"

#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <future>
#include <vector>

namespace test_app {

template <typename T>
class SLSTestMultithreaded : private SLSTestStandardHelper<T> {
public:
  using SLSTestStandardHelper<T>::SLSTestStandardHelper;

  ~SLSTestMultithreaded() override = default;

  /**
      @brief Creates threads that run sls operation
      1. Initialize SLS device, allocate tables.
      2. Run worker threads.
      3. Join threads.
  */
  void run(int num_engines, const std::vector<uint8_t> &tables,
           const SlsOpParams &sls_op_params,
           const std::filesystem::path &root) const;

  void print() const {
    fmt::print("SLSTestMultithreaded\n");
    this->print_params();
  }

  SlsOpParams generate_sls_run_params(const std::filesystem::path &root) const {
    return this->generate_sls_run_params_impl(root);
  }

private:
  /**
      @brief Run sls operation on worker threads
      1. Initialization: create contexts, generate run parameters.
      2. Synchronization: wait for other workers initialization to check real
      concurrency.
      3. Run SLS on SLS device/simulator.
      4. Golden check for results.
  */
  void worker(std::atomic<int> &visitor_cnt,
              const pnm::memory::EmbeddedTables *embedded_tables,
              const SlsOpParams &sls_op_params,
              const std::filesystem::path &root) const;
};

template <typename T>
void SLSTestMultithreaded<T>::run(int num_engines,
                                  const std::vector<uint8_t> &tables,
                                  const SlsOpParams &sls_op_params,
                                  const std::filesystem::path &root) const {

  pnm::memory::EmbeddedTables embedded_tables(
      pnm::view_cast<const uint32_t>(pnm::make_view(tables)),
      this->model_params_.tables_rows_num,
      this->model_params_.template feature_size_in_bytes<T>(), this->ctx_,
      this->testapp_run_params_.preference);

  std::atomic<int> visitor_cnt{num_engines};
  std::vector<std::future<void>> threads(num_engines);

  for (auto &thread : threads) {
    thread = std::async(std::launch::async, &SLSTestMultithreaded::worker, this,
                        std::ref(visitor_cnt), &embedded_tables,
                        std::ref(sls_op_params), root);
  }

  for (auto &thread : threads) {
    thread.get();
  }
}

template <typename T>
void SLSTestMultithreaded<T>::worker(
    std::atomic<int> &visitor_cnt,
    const pnm::memory::EmbeddedTables *embedded_tables,
    const SlsOpParams &sls_op_params, const std::filesystem::path &root) const {

  sls_start_profiling();
  auto sls_op = this->create_sls_op(embedded_tables);

  --visitor_cnt;
  while (visitor_cnt > 0) {
    ;
  }

  const auto psum_res = this->run_sls(sls_op, sls_op_params, this->runner_);

  const double error_ratio = this->perform_golden_check(psum_res, root);
  EXPECT_LE(error_ratio, kmismatch_ratio_threshold_val);

  sls_report_profile();
}

} // namespace test_app

#endif // _SLS_TEST_MULTITHREADED_H
