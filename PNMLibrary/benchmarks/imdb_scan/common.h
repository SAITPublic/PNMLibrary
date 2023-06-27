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

#ifndef PNM_BENCHMARK_COMMON_H
#define PNM_BENCHMARK_COMMON_H

#include "tools/datagen/imdb/selectivities_generator/factory.h"
#include "tools/datagen/imdb/utils.h"

#include "pnmlib/imdb/scan_types.h"

#include <benchmark/benchmark.h>

#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <tuple>
#include <vector>

namespace config {

inline constexpr auto REQUEST_COUNT = 10UL;

inline const std::vector<int64_t> BIT_SIZES{10, 18};
inline const std::vector<int64_t> COLUMN_SIZES{2'000'000UL, 2'000'000'000UL};
// Because gbench only supports integer arguments
inline const std::vector<int64_t> INVERSE_SELECTIVITIES{10, 1000};

inline const std::vector<std::vector<int64_t>> BENCH_PARAMS{
    BIT_SIZES, COLUMN_SIZES, INVERSE_SELECTIVITIES};
inline constexpr auto ITERATIONS_COUNT = 10;

inline constexpr auto MAX_THREAD_COUNT = 8U;

} // namespace config

template <pnm::imdb::OutputType output_type>
inline auto get_output_size(uint64_t column_size, double selectivity) {
  if (output_type == pnm::imdb::OutputType::IndexVector) {
    // Need a lot of leeway for smaller sizes. Considering small to be lower
    // than 200MB. If we're over that, then the selectivity is very accurate and
    // there's no need for much caution
    constexpr auto threshold = 200UL << 20;
    auto multiplier =
        static_cast<uint64_t>(column_size * selectivity) < threshold ? 2.5
                                                                     : 1.1;
    return static_cast<uint64_t>(column_size * selectivity * multiplier);
  }

  return column_size;
}

template <pnm::imdb::OperationType scan_operation>
using InputData = std::tuple<pnm::imdb::compressed_vector,
                             tools::gen::imdb::InputDataType<scan_operation>>;

template <pnm::imdb::OperationType scan_operation,
          pnm::imdb::OutputType scan_output_type>
inline auto make_input_data(uint64_t bit_size, uint64_t column_size,
                            double selectivity) {
  constexpr auto seed = 42;

  const auto generated_data =
      tools::gen::imdb::generate_and_store_data<scan_operation,
                                                scan_output_type>(
          std::filesystem::path{PATH_TO_TABLES},
          tools::gen::imdb::literals::selectivities_random_gen,
          tools::gen::imdb::literals::selectivities_random_gen, bit_size,
          column_size, config::REQUEST_COUNT, selectivity, false, seed);

  return InputData<scan_operation>{std::move(generated_data.column_),
                                   std::move(generated_data.input_)};
}

template <pnm::imdb::OperationType scan_operation,
          pnm::imdb::OutputType scan_output_type>
inline const auto &
make_input_data_cached(uint64_t bit_size, uint64_t column_size,
                       double selectivity, int thread_index) {
  struct Params {
    uint64_t bit_size;
    uint64_t column_size;
    double selectivity;

    bool operator==(const Params &other) const {
      return bit_size == other.bit_size && column_size == other.column_size &&
             selectivity == other.selectivity;
    }
    bool operator!=(const Params &other) const { return !(*this == other); }
  };

  static std::mutex mutex;
  static std::condition_variable data_loaded;
  static std::optional<InputData<scan_operation>> data;
  static Params stored_params{};

  {
    const std::lock_guard l(mutex);

    Params cur_params{bit_size, column_size, selectivity};

    if (stored_params != cur_params) {
      data.reset();
      stored_params = cur_params;
    }
  }

  if (thread_index == 0) {
    {
      const std::lock_guard l(mutex);
      data = make_input_data<scan_operation, scan_output_type>(
          bit_size, column_size, selectivity);
    }

    data_loaded.notify_all();
  } else {
    std::unique_lock l(mutex);
    data_loaded.wait(l, [] { return data.has_value(); });
  }

  return *data;
}

inline void extend_benchmark_values(benchmark::State &state,
                                    uint64_t column_size,
                                    uint64_t device_time) {
  static constexpr auto DEVICE_TIMER_FREQ = 125U;
  state.counters["Device time (us)"] = benchmark::Counter(
      device_time * DEVICE_TIMER_FREQ, benchmark::Counter::kAvgIterations,
      benchmark::Counter::OneK::kIs1000);

  auto elements_per_test = column_size * state.iterations() / 1'000'000;

  state.counters["Bandwidth (ME)"] =
      benchmark::Counter(elements_per_test, benchmark::Counter::kIsRate,
                         benchmark::Counter::OneK::kIs1000);

  state.counters["Throughput (QPS)"] =
      benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate,
                         benchmark::Counter::OneK::kIs1000);
}

inline void bench_setup(benchmark::internal::Benchmark *benchmark) {
  benchmark->ArgsProduct(config::BENCH_PARAMS);
  benchmark->ThreadRange(1, config::MAX_THREAD_COUNT);
  benchmark->Iterations(config::ITERATIONS_COUNT);
  benchmark->UseRealTime();
}

#endif // PNM_BENCHMARK_COMMON_H
