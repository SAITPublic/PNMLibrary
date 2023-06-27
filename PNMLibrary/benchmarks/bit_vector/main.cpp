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

// NOLINTBEGIN(misc-use-anonymous-namespace)

#include "pnmlib/imdb/bit_containers.h"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

inline constexpr auto BIT_SIZE = 10UL;
auto input_data(size_t count) {
  std::mt19937_64 eng{42}; // NOLINT(cert-msc32-c,cert-msc51-cpp)
  std::uniform_int_distribution<uint32_t> uni{0, (1UL << BIT_SIZE) - 1};
  std::vector<uint32_t> v(count);
  std::generate(v.begin(), v.end(), [&eng, &uni]() { return uni(eng); });
  return v;
}

inline constexpr auto N = 10000;

void bench_std_vector_bool_write(benchmark::State &state) {
  auto in = input_data(N);
  for ([[maybe_unused]] auto _ : state) {
    std::vector<bool> v(1024);
    for (auto e : in) {
      v[e] = true;
    }
    benchmark::DoNotOptimize(in);
  }
}
BENCHMARK(bench_std_vector_bool_write);

void bench_imdb_bitvector_write(benchmark::State &state) {
  auto in = input_data(N);
  for ([[maybe_unused]] auto _ : state) {
    pnm::containers::bit_vector v(1024);
    for (auto e : in) {
      v.insert(e);
    }
    benchmark::DoNotOptimize(v);
  }
}
BENCHMARK(bench_imdb_bitvector_write);

void bench_std_vector_bool_read(benchmark::State &state) {
  auto in = input_data(N);
  std::vector<bool> v(1024);
  for (auto e : in) {
    v[e] = true;
  }

  for ([[maybe_unused]] auto _ : state) {
    auto sum = 0UL;
    auto i = 0;
    for (auto e : v) {
      if (e == 1) {
        sum += i;
      }
      ++i;
    }
    benchmark::DoNotOptimize(sum);
  }
}
BENCHMARK(bench_std_vector_bool_read);

void bench_imdb_bitvector_read(benchmark::State &state) {
  auto in = input_data(N);
  const pnm::containers::bit_vector v(in.begin(), in.end());

  for ([[maybe_unused]] auto _ : state) {
    auto sum = 0;
    for (auto i = 0UL; i < v.size(); ++i) {
      if (v.contains(i)) {
        sum += i;
      }
    }
    benchmark::DoNotOptimize(sum);
  }
}
BENCHMARK(bench_imdb_bitvector_read);

void bench_std_vector_write(benchmark::State &state) {
  auto in = input_data(N);
  for ([[maybe_unused]] auto _ : state) {
    std::vector<uint32_t> v(in.size());
    for (auto i = 0; i < N; ++i) {
      v[i] = in[i];
    }
    benchmark::DoNotOptimize(v);
  }
}
BENCHMARK(bench_std_vector_write);

void bench_imdb_compressed_vector_write(benchmark::State &state) {
  auto in = input_data(N);
  for ([[maybe_unused]] auto _ : state) {
    pnm::containers::compressed_vector<uint32_t> v(BIT_SIZE, in.size());
    for (auto i = 0; i < N; ++i) {
      v[i] = in[i];
    }
    benchmark::DoNotOptimize(v);
  }
}
BENCHMARK(bench_imdb_compressed_vector_write);

void bench_std_vector_read(benchmark::State &state) {
  auto in = input_data(N);
  std::vector<uint32_t> v(std::move(in));
  for ([[maybe_unused]] auto _ : state) {
    auto sum = 0UL;
    for (auto i = 0; i < N; ++i) {
      sum += v[i];
    }
    benchmark::DoNotOptimize(sum);
  }
}
BENCHMARK(bench_std_vector_read);

void bench_imdb_compressed_vector_read(benchmark::State &state) {
  auto in = input_data(N);
  pnm::containers::compressed_vector<uint32_t> v(BIT_SIZE, in.begin(),
                                                 in.end());
  for ([[maybe_unused]] auto _ : state) {
    auto sum = 0UL;
    for (auto i = 0; i < N; ++i) {
      sum += v[i];
    }
    benchmark::DoNotOptimize(sum);
  }
}
BENCHMARK(bench_imdb_compressed_vector_read);

BENCHMARK_MAIN();

// NOLINTEND(misc-use-anonymous-namespace)
