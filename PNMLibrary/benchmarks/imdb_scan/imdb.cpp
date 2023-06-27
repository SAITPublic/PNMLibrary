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

// NOLINTBEGIN(misc-use-anonymous-namespace)

#include "common.h"

#include "pnmlib/imdb/scan.h"
#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/core/allocator.h"
#include "pnmlib/core/buffer.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/runner.h"

#include "pnmlib/common/views.h"

#include <benchmark/benchmark.h>

#include <linux/imdb_resources.h>

#include <array>
#include <cstdint>
#include <type_traits>

// Data must be on different ranks for HW parallelism (rank 1 = lower 16GB, rank
// 2 = upper 16GB)

// Each column takes up max 4.2 GB (for 2e9 elements, 18 bit size)
// NOTE: To enable parallelism when only one HW thread is used, we put the first
// table column into rank 1 and the first result + predictor into rank 2. This
// way the first thread is able to get a bit more throughput. This doesn't
// matter when there are more threads, as they will consume all available
// throughput even without this.
// Each predictor takes up max 32 KB
// Each result takes up max 800 MB (in case of IndexVector)

using namespace pnm::imdb;
using namespace pnm::memory;

namespace {

std::array<Buffer<uint32_t>, IMDB_NUM_OF_RANK> column_buffer;

template <typename BitContainer> auto make_view(BitContainer &c) {
  return pnm::make_view(c.container());
}

template <> auto make_view(index_vector &c) { return pnm::make_view(c); }

void release_column_buffer_if(int thread_idx) {
  if (thread_idx < IMDB_NUM_OF_RANK) {
    column_buffer[thread_idx] = Buffer<uint32_t>();
  }
}

} // namespace

template <OutputType output_type> void imdb_in_range(benchmark::State &state) {
  const uint64_t bit_size = state.range(0);
  const uint64_t column_size = state.range(1);
  const double selectivity = 1.0 / state.range(2);
  const uint8_t rank = state.thread_index() % IMDB_NUM_OF_RANK;
  const uint8_t other_rank = (rank + 1) % IMDB_NUM_OF_RANK;
  const auto thread_idx = state.thread_index();
  auto &column_buf = column_buffer[rank];

  const auto &[column, input] =
      ::make_input_data_cached<OperationType::InRange, output_type>(
          bit_size, column_size, selectivity, thread_idx);

  static auto context = pnm::make_context(pnm::Device::Type::IMDB_CXL);

  release_column_buffer_if(thread_idx);

  if (thread_idx < IMDB_NUM_OF_RANK) {
    column_buf = Buffer(::make_view(column), context, property::CURegion(rank));
  }

  using result_type = std::conditional_t<output_type == OutputType::BitVector,
                                         bit_vector, index_vector>;
  result_type result(get_output_size<output_type>(column_size, selectivity));
  auto result_buf =
      Buffer(::make_view(result), context, property::CURegion(other_rank));

  const pnm::Runner runner(context);

  uint64_t device_time = 0;

  auto request_id = 0U;
  for ([[maybe_unused]] auto _ : state) {
    pnm::operations::Scan op({bit_size, column.size(), &column_buf},
                             input[request_id], &result_buf, output_type);

    request_id = (request_id + 1) % input.size();

    runner.run(op);

    device_time += op.execution_time();

    // Skip copying results - not part of the benchmark
  }

  extend_benchmark_values(state, column_size, device_time);
}

template <OutputType output_type> void imdb_in_list(benchmark::State &state) {
  const uint64_t bit_size = state.range(0);
  const uint64_t column_size = state.range(1);
  const double selectivity = 1.0 / state.range(2);
  const uint8_t rank = state.thread_index() % IMDB_NUM_OF_RANK;
  const uint8_t other_rank = (rank + 1) % IMDB_NUM_OF_RANK;
  const auto thread_idx = state.thread_index();
  auto &column_buf = column_buffer[rank];

  const auto &[column, input] =
      ::make_input_data_cached<OperationType::InList, OutputType::BitVector>(
          bit_size, column_size, selectivity, state.thread_index());

  static auto context = pnm::make_context(pnm::Device::Type::IMDB_CXL);

  release_column_buffer_if(thread_idx);

  if (thread_idx < IMDB_NUM_OF_RANK) {
    column_buf = Buffer(::make_view(column), context, property::CURegion(rank));
  }

  using result_type = std::conditional_t<output_type == OutputType::BitVector,
                                         bit_vector, index_vector>;
  result_type result(get_output_size<output_type>(column_size, selectivity));
  auto result_buf =
      Buffer(::make_view(result), context, property::CURegion(other_rank));

  bit_vector predictor(1U << bit_size);
  auto predictor_buf = Buffer(pnm::make_view(predictor.container()), context,
                              property::CURegion(other_rank));

  const pnm::Runner runner(context);

  uint64_t device_time = 0;

  auto request_id = 0;
  for ([[maybe_unused]] auto _ : state) {
    predictor_buf.bind_user_region(pnm::view_const_cast<uint32_t>(
        pnm::make_view(input[request_id].container())));
    predictor_buf.copy_to_device();

    pnm::operations::Scan op({bit_size, column.size(), &column_buf},
                             {input[request_id].size() + 1, &predictor_buf},
                             &result_buf, output_type);

    request_id = (request_id + 1) % input.size();

    runner.run(op);

    device_time += op.execution_time();

    // Skip copying results - not part of the benchmark
  }

  extend_benchmark_values(state, column_size, device_time);
}

class IMDBEngine; // Dummy class to match output with AVX2 and NoSIMD benchmarks
template <OperationType scan_operation, OutputType scan_output_type,
          typename Engine>
void Scan(benchmark::State &state) {
  static_assert(std::is_same_v<Engine, IMDBEngine>,
                "Only IMDBEngine class is supported");
  if constexpr (scan_operation == OperationType::InList) {
    imdb_in_list<scan_output_type>(state);
  } else {
    imdb_in_range<scan_output_type>(state);
  }
}

BENCHMARK(Scan<OperationType::InRange, OutputType::BitVector, IMDBEngine>)
    ->Apply(bench_setup);

BENCHMARK(Scan<OperationType::InRange, OutputType::IndexVector, IMDBEngine>)
    ->Apply(bench_setup);

BENCHMARK(Scan<OperationType::InList, OutputType::BitVector, IMDBEngine>)
    ->Apply(bench_setup);

BENCHMARK(Scan<OperationType::InList, OutputType::IndexVector, IMDBEngine>)
    ->Apply(bench_setup);

// NOLINTEND(misc-use-anonymous-namespace)
