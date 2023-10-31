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
#include "common/log.h"
#include "common/make_error.h"
#include "common/profile.h"

// Internal headers for data generation
#include "tools/datagen/imdb/column_generator/factory.h"
#include "tools/datagen/imdb/range_generator/factory.h"
#include "tools/datagen/imdb/utils.h"

#include "pnmlib/imdb/scan.h"
#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/core/buffer.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/runner.h"

#include "pnmlib/common/views.h"

#include "CLI/App.hpp"
#include "CLI/CLI.hpp" // NOLINT(misc-include-cleaner)
#include "CLI/Validators.hpp"

#include <fmt/core.h>

#include <spdlog/common.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <future>
#include <ratio>
#include <utility>
#include <vector>

namespace {

std::filesystem::path data_root;

inline constexpr auto column_gen_name =
    tools::gen::imdb::literals::column_random_generator;
inline constexpr auto ranges_gen_name =
    tools::gen::imdb::literals::scan_ranges_generator_random;
inline constexpr auto bit_compression = 9;
inline constexpr auto column_size = 500'000;
inline constexpr auto inputs_size = 5'000;
inline constexpr auto selectivity = 0.1;

inline constexpr auto device_timer_freq_ms = 0.125;

} // namespace

// InRange operation with index vector output
uint64_t worker_job_iv(const pnm::imdb::compressed_vector &column,
                       const pnm::imdb::Ranges &ranges,
                       const pnm::imdb::IndexVectors &golden_results) {
  uint64_t device_time = 0;
  auto context = pnm::make_context(pnm::Device::Type::IMDB);
  const pnm::Runner runner(context);
  const pnm::memory::Buffer<uint32_t> column_buf(
      pnm::views::make_view(column.container()), context);
  pnm::imdb::IndexVectors results;

  for (const auto &range : ranges) {
    pnm::imdb::index_vector result(column.size() + 1);
    pnm::memory::Buffer<uint32_t> result_buf(pnm::views::make_view(result),
                                             context);

    pnm::operations::Scan op(
        pnm::operations::Scan::Column{column.value_bits(), column.size(),
                                      &column_buf},
        range, &result_buf, pnm::imdb::OutputType::IndexVector);
    runner.run(op);

    result_buf.copy_from_device();
    result.resize(op.result_size());
    results.push_back(std::move(result));

    device_time += op.execution_time();
  }

  if (!std::equal(golden_results.begin(), golden_results.end(), results.begin(),
                  results.end())) {
    throw pnm::error::make_fail(
        "Operation result does not equal to the golden result!\n");
  }
  return device_time;
}

// InRange operation with bit vector output
uint64_t worker_job_bv(const pnm::imdb::compressed_vector &column,
                       const pnm::imdb::Ranges &ranges,
                       const pnm::imdb::BitVectors &golden_results) {
  uint64_t device_time = 0;
  auto context = pnm::make_context(pnm::Device::Type::IMDB);
  const pnm::Runner runner(context);
  const pnm::memory::Buffer<uint32_t> column_buf(
      pnm::views::make_view(column.container()), context);
  pnm::imdb::BitVectors results;

  for (const auto &range : ranges) {
    pnm::imdb::bit_vector result(column.size() + 1);
    pnm::memory::Buffer<uint32_t> result_buf(
        pnm::views::make_view(result.container()), context);

    pnm::operations::Scan op(
        pnm::operations::Scan::Column{column.value_bits(), column.size(),
                                      &column_buf},
        range, &result_buf, pnm::imdb::OutputType::BitVector);
    runner.run(op);

    result_buf.copy_from_device();
    results.push_back(std::move(result));

    device_time += op.execution_time();
  }

  if (!std::equal(golden_results.begin(), golden_results.end(), results.begin(),
                  results.end())) {
    throw pnm::error::make_fail(
        "Operation result does not equal to the golden result!\n");
  }
  return device_time;
}

void print_results(uint64_t real_time_ms, size_t workers_cnt) {
  const double real_time_s_inv =
      static_cast<double>(std::milli::den) / real_time_ms;
  fmt::print("Real time (ms): {}\n", real_time_ms);
  fmt::print("Bandwidth (MEPS): {:.3f}\n", real_time_s_inv * workers_cnt *
                                               ::column_size * ::inputs_size /
                                               std::mega::num);
  fmt::print("Throughput (QPS): {:.3f}\n",
             real_time_s_inv * workers_cnt * ::inputs_size);
  fmt::print("\n");
}

template <pnm::imdb::OutputType scan_output_type>
void main_job(size_t workers_cnt) {
  fmt::print("Start {} job(s), output type - {}...\n", workers_cnt,
             (scan_output_type == pnm::imdb::OutputType::BitVector
                  ? "bit vectors"
                  : "index vectors"));

  // generate table, ranges and golden vectors
  const auto [column, input, golden] =
      tools::gen::imdb::generate_and_store_data<
          pnm::imdb::OperationType::InRange, scan_output_type>(
          ::data_root, ::column_gen_name, ::ranges_gen_name, ::bit_compression,
          ::column_size, ::inputs_size, ::selectivity,
          /* generate_golden = */ true);

  pnm::profile::TimerMsT timer;
  // create N workers
  std::vector<std::future<uint64_t>> workers(workers_cnt);
  for (auto &worker : workers) {
    if constexpr (scan_output_type == pnm::imdb::OutputType::BitVector) {
      worker = std::async(std::launch::async, worker_job_bv, std::cref(column),
                          std::cref(input), std::cref(golden));
    } else {
      worker = std::async(std::launch::async, worker_job_iv, std::cref(column),
                          std::cref(input), std::cref(golden));
    }
  }
  timer.tick();

  // wait for all workers to complete
  for (size_t i = 0; i < workers_cnt; ++i) {
    auto worker_device_time = workers[i].get();
    fmt::print("#{} worker's device time (ms): {}\n", i + 1,
               worker_device_time * ::device_timer_freq_ms);
  }
  timer.tock();

  print_results(timer.duration().count(), workers_cnt);
}

int main(int argc, char **argv) {
  CLI::App app{"IMDB demo app"};

  app.add_option("data_root", ::data_root,
                 "Path to folder where data will be generated")
      ->required()
      ->check(CLI::ExistingDirectory);

  CLI11_PARSE(app, argc, argv);

  pnm::log::set_active_level(spdlog::level::level_enum::warn);
  main_job<pnm::imdb::OutputType::BitVector>(1);
  main_job<pnm::imdb::OutputType::BitVector>(5);
  main_job<pnm::imdb::OutputType::IndexVector>(1);
  main_job<pnm::imdb::OutputType::IndexVector>(5);

  return 0;
}
