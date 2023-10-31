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

// NOLINTBEGIN(misc-use-anonymous-namespace)

#include "benchmarks/imdb_scan/nosimd_baseline.h"
#include "common.h"

#include "imdb/software_scan/avx2/scan.h"

#include "pnmlib/imdb/scan_types.h"

#include <benchmark/benchmark.h>

#include <cstdint>

using namespace pnm::imdb;

template <typename Output> auto make_view(Output &output) {
  if constexpr (std::is_same_v<Output, index_vector>) {
    return pnm::views::make_view(output);
  } else {
    return output.view();
  }
}

template <typename Args> auto forward_args(const Args &arg) {
  if constexpr (std::is_same_v<Args, RangeOperation>) {
    return arg;
  } else {
    return arg.view();
  }
}

template <OperationType scan_operation, OutputType scan_output_type,
          typename Engine>
void scan_bench_impl(benchmark::State &state) {
  const uint64_t bit_size = state.range(0);
  const uint64_t column_size = state.range(1);
  const double selectivity = 1.0 / state.range(2);

  const auto &[column, input] =
      ::make_input_data_cached<scan_operation, scan_output_type>(
          bit_size, column_size, selectivity, state.thread_index());

  OutputContainer<scan_output_type> output(
      get_output_size<scan_output_type>(column_size, selectivity));
  const Engine eng;

  auto iv = ::make_view(output);

  auto request_id = 0U;
  for ([[maybe_unused]] auto _ : state) {
    if constexpr (std::is_same_v<Engine, NoSIMDBaseline>) {
      auto count = eng.scan(column, input[request_id], output);
      benchmark::DoNotOptimize(count);
    } else {
      auto column_view = std::cref(column).get().view();
      auto count = eng.scan(column_view, forward_args(input[request_id]), iv);
      benchmark::DoNotOptimize(count);
    }
    benchmark::DoNotOptimize(iv);
  }

  extend_benchmark_values(state, column_size, 0);
}

template <OperationType scan_operation, OutputType scan_output_type,
          typename Engine>
void Scan(benchmark::State &state) {
  scan_bench_impl<scan_operation, scan_output_type, Engine>(state);
}

struct AVX2Engine {
  template <typename... Forward> auto scan(Forward &&...args) const {
    return internal::avx2::scan(std::forward<Forward>(args)...);
  }
};

BENCHMARK(Scan<OperationType::InRange, OutputType::BitVector, NoSIMDBaseline>)
    ->Apply(bench_setup);
BENCHMARK(Scan<OperationType::InRange, OutputType::IndexVector, NoSIMDBaseline>)
    ->Apply(bench_setup);
BENCHMARK(Scan<OperationType::InList, OutputType::BitVector, NoSIMDBaseline>)
    ->Apply(bench_setup);
BENCHMARK(Scan<OperationType::InList, OutputType::IndexVector, NoSIMDBaseline>)
    ->Apply(bench_setup);

BENCHMARK(Scan<OperationType::InRange, OutputType::BitVector, AVX2Engine>)
    ->Apply(bench_setup);
BENCHMARK(Scan<OperationType::InRange, OutputType::IndexVector, AVX2Engine>)
    ->Apply(bench_setup);
BENCHMARK(Scan<OperationType::InList, OutputType::BitVector, AVX2Engine>)
    ->Apply(bench_setup);
BENCHMARK(Scan<OperationType::InList, OutputType::IndexVector, AVX2Engine>)
    ->Apply(bench_setup);

BENCHMARK_MAIN();

// NOLINTEND(misc-use-anonymous-namespace)
