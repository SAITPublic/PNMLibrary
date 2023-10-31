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

#ifndef _SLS_TEST_MULTIPROCESS_H
#define _SLS_TEST_MULTIPROCESS_H

#include "standard_helper.h"

#include "test/mocks/context/utils.h"
#include "test/sls_app/api/constants.h"
#include "test/sls_app/api/structs.h"

#include "pnmlib/sls/embedding_tables.h"
#include "pnmlib/sls/sls.h"

#include "pnmlib/core/runner.h"

#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <signal.h> // NOLINT(modernize-deprecated-headers)
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace test_app {

template <typename T>
class SlsTestMultiprocess : private SlsTestStandardHelper<T> {
public:
  using SlsTestStandardHelper<T>::SlsTestStandardHelper;

  ~SlsTestMultiprocess() override = default;

  /**
      @brief Parent process creates child processes which run sls operation
      1. Initialize SLS device and allocate tables.
      2. Fork child processes only if tables are correctly allocated.
      3. Wait for all of children to complete task.
      4. Kill child processes.
  */
  void run(int num_engines, const std::vector<uint8_t> &tables,
           const SlsOpParams &sls_op_params,
           const std::filesystem::path &root) const;

  void print() const {
    fmt::print("SlsTestMultiprocess\n");
    this->print_params();
  }

  SlsOpParams generate_sls_run_params(const std::filesystem::path &root) const {
    return this->generate_sls_run_params_impl(root);
  }

private:
  /**
      @brief Perform sls operation for child processes
      1. Initialization: create contexts, generate run parameters.
      2. Run SLS on SLS device/simulator.
      3. Golden check for results.
  */
  void
  child_process(const SlsOpParams &sls_op_params,
                const std::filesystem::path &root,
                const pnm::memory::EmbeddingTables *embedding_tables) const;

  void child_process_impl(
      const SlsOpParams &sls_op_params, const std::filesystem::path &root,
      const pnm::memory::EmbeddingTables *embedding_tables) const;
};

template <typename T>
void SlsTestMultiprocess<T>::run(int num_engines,
                                 const std::vector<uint8_t> &tables,
                                 const SlsOpParams &sls_op_params,
                                 const std::filesystem::path &root) const {
  pnm::profile::sls_start_profiling();

  pnm::memory::EmbeddingTables embedding_tables(
      pnm::views::make_view(tables), this->model_params_.tables_rows_num,
      this->model_params_.template feature_size_in_bytes<T>(), this->ctx_,
      this->testapp_run_params_.preference);

  pnm::profile::sls_report_profile();
  pnm::profile::sls_end_profiling();

  std::vector<pid_t> children_pids; // NOLINT(misc-include-cleaner)
  children_pids.reserve(num_engines);

  int child_index;
  pid_t pid{};
  for (child_index = 0; child_index < num_engines; ++child_index) {
    pid = fork();
    if (pid == 0) {
      break;
    }
    if (pid == -1) {
      throw std::runtime_error(
          fmt::format("Failed to create a child process. Error is '{}'.",
                      std::strerror(errno)));
    }

    children_pids.push_back(pid);
  }

  if (pid == 0) {
    // child

    // Run SLS workload
    child_process(sls_op_params, root, &embedding_tables);
  } else {
    // parent
    bool error_happened = false;

    for (size_t i = 0; i < children_pids.size() && !error_happened; ++i) {
      int status;
      auto child_pid = wait(&status);
      EXPECT_EQ(status, 0) << "Child " << child_pid
                           << " failed to finish correctly: "
                           << std::strerror(errno) << '\n';

      if (status != 0) {
        error_happened = true;
        break;
      }
    }

    if (error_happened) {
      for (auto child_pid : children_pids) {
        kill(child_pid, SIGKILL);
        waitpid(child_pid, nullptr, 0);
      }
    }
  }
}

template <typename T>
void SlsTestMultiprocess<T>::child_process_impl(
    const SlsOpParams &sls_op_params, const std::filesystem::path &root,
    const pnm::memory::EmbeddingTables *embedding_tables) const {
  pnm::profile::sls_start_profiling();
  {
    auto ctx = test::mock::make_context_mock(this->run_params_.context_type);
    pnm::Runner runner{ctx};
    auto sls_op = this->create_sls_op(embedding_tables);

    const auto psum_res = this->run_sls(sls_op, sls_op_params, runner);

    const double error_ratio = this->perform_golden_check(psum_res, root);
    EXPECT_LE(error_ratio, kmismatch_ratio_threshold_val);
  }
  pnm::profile::sls_report_profile();
}

template <typename T>
void SlsTestMultiprocess<T>::child_process(
    const SlsOpParams &sls_op_params, const std::filesystem::path &root,
    const pnm::memory::EmbeddingTables *embedding_tables) const {

  // This macro is necessary to prevent a fork-bomb explosion due to the
  // release of an unhandled exception.
  EXPECT_NO_THROW(child_process_impl(sls_op_params, root, embedding_tables));

  // Terminate child process
  // So destructors of parent's scoped objects are not called.
  exit(testing::Test::HasFailure());
}

} // namespace test_app

#endif // _SLS_TEST_MULTIPROCESS_H
