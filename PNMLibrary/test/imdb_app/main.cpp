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

#include "imdb.h"

#include "imdb/utils/runner/avx2_scan.h"
#include "imdb/utils/runner/cpu_scan.h"

#include "tools/datagen/imdb/column_generator/factory.h"
#include "tools/datagen/imdb/predicate_vec_generator/factory.h"
#include "tools/datagen/imdb/range_generator/factory.h"
#include "tools/datagen/imdb/utils.h"

#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-port.h>

#include "CLI/App.hpp"
#include "CLI/CLI.hpp" // NOLINT(misc-include-cleaner)
#include "CLI/Validators.hpp"

#include <fmt/core.h>

#include <sched.h>
#include <signal.h> // NOLINT(modernize-deprecated-headers)
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <future>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using namespace test_app;
using namespace pnm::imdb;
using namespace tools::gen::imdb;

namespace {
size_t num_threads = 0;
} // namespace

static constexpr size_t max_threads = 50;
static constexpr bool IMDB_RUNNER_MODE = true;
static constexpr bool AVX2_RUNNER_MODE = false;

template <OperationType operation, OutputType output_type, typename Input,
          typename Runner>
auto DispatchOperation(Runner &runner, const Input &input) {
  if constexpr (operation == OperationType::InRange &&
                output_type == OutputType::BitVector) {
    return runner.compute_in_range_to_bv(input);
  } else if constexpr (operation == OperationType::InRange &&
                       output_type == OutputType::IndexVector) {
    return runner.compute_in_range_to_iv(input);
  } else if constexpr (operation == OperationType::InList &&
                       output_type == OutputType::BitVector) {
    return runner.compute_in_list_to_bv(input);
  } else if constexpr (operation == OperationType::InList &&
                       output_type == OutputType::IndexVector) {
    return runner.compute_in_list_to_iv(input);
  } else {
    throw std::runtime_error("Got invalid operation or output_type!");
  }
}

template <OperationType scan_operation, OutputType scan_output_type>
void RunOperation(const compressed_vector &column,
                  const InputDataType<scan_operation> &input,
                  const bool runner_mode,
                  const OutputDataType<scan_output_type> &golden_result,
                  pnm::ContextHandler &context) {
  OutputDataType<scan_output_type> result;

  if (runner_mode == IMDB_RUNNER_MODE) {
    ImdbRunner runner(column, context);
    result = DispatchOperation<scan_operation, scan_output_type>(runner, input);
  } else {
    AVX2Runner runner(column);
    result = DispatchOperation<scan_operation, scan_output_type>(runner, input);
  }
  ASSERT_TRUE(std::equal(result.begin(), result.end(), golden_result.begin(),
                         golden_result.end()));
}

template <OperationType scan_operation, OutputType scan_output_type>
class ImdbTestAppMT : public testing::TestWithParam<
                          std::tuple<bool, std::string, std::string, uint64_t,
                                     index_type, uint64_t, double>> {
public:
  void run(size_t num_threads) {
    auto [runner_mode, column_gen_name, pred_gen_name, bit_size, column_size,
          inputs_size, selectivity] = GetParam();
    const auto [column, input, result] =
        generate_and_store_data<scan_operation, scan_output_type, CPURunner>(
            std::filesystem::path{PATH_TO_TABLES}, column_gen_name,
            pred_gen_name, bit_size, column_size, inputs_size, selectivity,
            true);

    if (runner_mode == IMDB_RUNNER_MODE) {
      context_ = pnm::make_context(pnm::Device::Type::IMDB);
    }

    // [TODO: MCS23-432] use std::barrier when c++20 become true
    std::atomic<size_t> visitor_cnt{num_threads};
    std::vector<std::future<void>> threads(num_threads);

    for (auto &thread : threads) {
      thread = std::async(std::launch::async, &ImdbTestAppMT::worker, this,
                          std::cref(column), std::cref(input), runner_mode,
                          std::cref(result), std::ref(visitor_cnt));
    }

    for (auto &thread : threads) {
      thread.get();
    }
  }

private:
  void worker(const compressed_vector &column,
              const InputDataType<scan_operation> &input,
              const bool runner_mode,
              const OutputDataType<scan_output_type> &golden_result,
              std::atomic<size_t> &visitor_cnt) {

    --visitor_cnt;
    while (visitor_cnt > 0) {
      ;
    }

    RunOperation<scan_operation, scan_output_type>(column, input, runner_mode,
                                                   golden_result, context_);
  }

  pnm::ContextHandler context_;
};

template <OperationType scan_operation, OutputType scan_output_type>
class ImdbTestAppMP : public testing::TestWithParam<
                          std::tuple<bool, std::string, std::string, uint64_t,
                                     index_type, uint64_t, double>> {
public:
  void run(size_t num_engines) {
    auto [runner_mode, column_gen_name, pred_gen_name, bit_size, column_size,
          inputs_size, selectivity] = GetParam();
    const auto [column, input, result] =
        generate_and_store_data<scan_operation, scan_output_type, CPURunner>(
            std::filesystem::path{PATH_TO_TABLES}, column_gen_name,
            pred_gen_name, bit_size, column_size, inputs_size, selectivity,
            true);

    std::vector<pid_t> children_pids;
    children_pids.reserve(num_engines);

    pid_t pid{};
    for (size_t child_index = 0; child_index < num_engines; ++child_index) {
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
      child_process(column, input, runner_mode, result);
    } else {
      // parent
      bool error_happened = false;

      for (size_t i = 0; i < children_pids.size() && !error_happened; ++i) {
        int status;
        auto child_pid = wait(&status);
        EXPECT_EQ(status, 0)
            << "Child " << child_pid
            << " failed to finish correctly: " << std::strerror(errno) << '\n';

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

private:
  void
  child_process_impl(const compressed_vector &column,
                     const InputDataType<scan_operation> &input,
                     const bool runner_mode,
                     const OutputDataType<scan_output_type> &golden_result) {

    if (runner_mode == IMDB_RUNNER_MODE) {
      context_ = pnm::make_context(pnm::Device::Type::IMDB);
    }

    RunOperation<scan_operation, scan_output_type>(column, input, runner_mode,
                                                   golden_result, context_);
  }

  void child_process(const compressed_vector &column,
                     const InputDataType<scan_operation> &input,
                     const bool runner_mode,
                     const OutputDataType<scan_output_type> &golden_result) {

    // This macro is necessary to prevent a fork-bomb explosion due to the
    // release of an unhandled exception.
    EXPECT_NO_THROW(
        child_process_impl(column, input, runner_mode, golden_result));

    exit(testing::Test::HasFailure());
  }

  pnm::ContextHandler context_;
};

// In order the params are:
// 1. column_gen_name (for example,
// literals::column_positioned_generator)
// 2. pred_gen_name (for example, literals::scan_ranges_generator_random)
// 3. bit_size = amount of bits per element in compressed column
// 4. column_size = amount of elements in column
// 5. inputs_size = amount of operations to be performed (e.g. N distinct
// InRange operations)
// 6. selectivity = the fraction of entries in column that fit the
// predicate
auto testing_parameters_in_range =
    testing::Combine(testing::Values(AVX2_RUNNER_MODE, IMDB_RUNNER_MODE),
                     testing::Values(literals::column_random_generator),
                     testing::Values(literals::scan_ranges_generator_random),
                     testing::Values(10), testing::Values(10'000),
                     testing::Values(100), testing::Values(0.5));
auto testing_parameters_in_list = testing::Combine(
    testing::Values(AVX2_RUNNER_MODE, IMDB_RUNNER_MODE),
    testing::Values(literals::column_random_generator),
    testing::Values(literals::predictor_generator_random), testing::Values(10),
    testing::Values(10'000), testing::Values(100), testing::Values(0.5));

auto stress_testing_parameters_in_range = testing::Combine(
    testing::Values(AVX2_RUNNER_MODE, IMDB_RUNNER_MODE),
    testing::Values(literals::column_random_generator,
                    literals::column_positioned_generator),
    testing::Values(literals::scan_ranges_generator_random),
    testing::Values(2, 5, 6, 7, 9, 10, 12, 16, 17, 18),
    testing::Values(100'000), testing::Values(1'000), testing::Values(0.5));
auto stress_testing_parameters_in_list = testing::Combine(
    testing::Values(AVX2_RUNNER_MODE, IMDB_RUNNER_MODE),
    testing::Values(literals::column_random_generator,
                    literals::column_positioned_generator),
    testing::Values(literals::predictor_generator_random),
    testing::Values(2, 5, 6, 7, 9, 10, 12, 16, 17, 18),
    testing::Values(100'000), testing::Values(1'000), testing::Values(0.5));

using InRangeToBVTestAppMT =
    ImdbTestAppMT<OperationType::InRange, OutputType::BitVector>;
using InRangeToIVTestAppMT =
    ImdbTestAppMT<OperationType::InRange, OutputType::IndexVector>;
using InListToBVTestAppMT =
    ImdbTestAppMT<OperationType::InList, OutputType::BitVector>;
using InListToIVTestAppMT =
    ImdbTestAppMT<OperationType::InList, OutputType::IndexVector>;

TEST_P(InRangeToBVTestAppMT, Main) { this->run(num_threads); }
TEST_P(InRangeToIVTestAppMT, Main) { this->run(num_threads); }
TEST_P(InListToBVTestAppMT, Main) { this->run(num_threads); }
TEST_P(InListToIVTestAppMT, Main) { this->run(num_threads); }

INSTANTIATE_TEST_SUITE_P(ImdbTestMT, InRangeToBVTestAppMT,
                         testing_parameters_in_range);
INSTANTIATE_TEST_SUITE_P(ImdbTestMT, InRangeToIVTestAppMT,
                         testing_parameters_in_range);
INSTANTIATE_TEST_SUITE_P(ImdbTestMT, InListToBVTestAppMT,
                         testing_parameters_in_list);
INSTANTIATE_TEST_SUITE_P(ImdbTestMT, InListToIVTestAppMT,
                         testing_parameters_in_list);

INSTANTIATE_TEST_SUITE_P(ImdbTestStressMT, InRangeToBVTestAppMT,
                         stress_testing_parameters_in_range);
INSTANTIATE_TEST_SUITE_P(ImdbTestStressMT, InRangeToIVTestAppMT,
                         stress_testing_parameters_in_range);
INSTANTIATE_TEST_SUITE_P(ImdbTestStressMT, InListToBVTestAppMT,
                         stress_testing_parameters_in_list);
INSTANTIATE_TEST_SUITE_P(ImdbTestStressMT, InListToIVTestAppMT,
                         stress_testing_parameters_in_list);

using InRangeToBVTestAppMP =
    ImdbTestAppMP<OperationType::InRange, OutputType::BitVector>;
using InRangeToIVTestAppMP =
    ImdbTestAppMP<OperationType::InRange, OutputType::IndexVector>;
using InListToBVTestAppMP =
    ImdbTestAppMP<OperationType::InList, OutputType::BitVector>;
using InListToIVTestAppMP =
    ImdbTestAppMP<OperationType::InList, OutputType::IndexVector>;

TEST_P(InRangeToBVTestAppMP, Main) { this->run(num_threads); }
TEST_P(InRangeToIVTestAppMP, Main) { this->run(num_threads); }
TEST_P(InListToBVTestAppMP, Main) { this->run(num_threads); }
TEST_P(InListToIVTestAppMP, Main) { this->run(num_threads); }

INSTANTIATE_TEST_SUITE_P(ImdbTestMP, InRangeToBVTestAppMP,
                         testing_parameters_in_range);
INSTANTIATE_TEST_SUITE_P(ImdbTestMP, InRangeToIVTestAppMP,
                         testing_parameters_in_range);
INSTANTIATE_TEST_SUITE_P(ImdbTestMP, InListToBVTestAppMP,
                         testing_parameters_in_list);
INSTANTIATE_TEST_SUITE_P(ImdbTestMP, InListToIVTestAppMP,
                         testing_parameters_in_list);

INSTANTIATE_TEST_SUITE_P(ImdbTestStressMP, InRangeToBVTestAppMP,
                         stress_testing_parameters_in_range);
INSTANTIATE_TEST_SUITE_P(ImdbTestStressMP, InRangeToIVTestAppMP,
                         stress_testing_parameters_in_range);
INSTANTIATE_TEST_SUITE_P(ImdbTestStressMP, InListToBVTestAppMP,
                         stress_testing_parameters_in_list);
INSTANTIATE_TEST_SUITE_P(ImdbTestStressMP, InListToIVTestAppMP,
                         stress_testing_parameters_in_list);

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv); // removes all recognized Gtest flags

  for (const std::string_view arg : testing::internal::GetArgvs()) {
    if (arg == "--gtest_list_tests") {
      // Gtest dummy test run by CMake to dynamically define tests at runtime
      return RUN_ALL_TESTS();
    }
  }

  CLI::App app{"IMDB test application"};

  app.add_option("num_threads", num_threads,
                 "Number of threads/processes to start up")
      ->required()
      ->check(CLI::Range(1UL, max_threads));

  CLI11_PARSE(app, argc, argv);

  return RUN_ALL_TESTS();
}
