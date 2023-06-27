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

#include "secure/encryption/AES/common.h"
#include "secure/encryption/sls_engine.h"

#include "common/sls_types.h"

#include "pnmlib/common/views.h"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <numeric>
#include <vector>

auto key() {
  static constexpr auto aes_key_size =
      static_cast<int>(sls::secure::AES_KEY_SIZE::AES_128) / 8;
  static constexpr auto key_str = "12345678901234561234567890123456";
  sls::FixedVector<uint8_t, aes_key_size> key;
  std::copy_n(key_str, aes_key_size, key.begin());
  return key;
}

auto indices_vec(int icount = 40, uintptr_t value = 10) {
  std::vector<uintptr_t> indices(icount);
  std::iota(indices.begin(), indices.end(), value);
  return indices;
}

inline constexpr auto sparce_feature_size = 16;

constexpr auto sls_op = [](auto sum, const auto &e) {
  std::transform(sum.begin(), sum.end(), e.begin(), sum.begin(), std::plus{});
  return sum;
};

void benchmark_core_old(int minibatch_size, int num_lookup,
                        benchmark::State &state) {
  const sls::secure::EncryptionEngine<uint32_t> engine(42, key());

  std::vector<uint32_t> result(sparce_feature_size, 0U);

  std::vector<std::vector<uintptr_t>> batches;
  batches.reserve(minibatch_size);
  for (auto i = 0; i < minibatch_size; ++i) {
    batches.emplace_back(indices_vec(num_lookup, num_lookup + i));
  }

  for ([[maybe_unused]] auto _ : state) {
    for (auto &indices : batches) {
#pragma GCC diagnostic push // Clang accept this too??
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
      engine.offset_transform_reduce(&(*indices.begin()), &(*indices.end()),
                                     pnm::make_view(result), sls_op);
#pragma GCC diagnostic pop
      benchmark::DoNotOptimize(result);
    }
  }
  auto v = std::accumulate(result.begin(), result.end(), 0U);
  benchmark::DoNotOptimize(v);
}

void benchmark_core(int minibatch_size, int num_lookup,
                    benchmark::State &state) {
  const sls::secure::EncryptionEngine<uint32_t> engine(42, key());

  std::vector<uint32_t> result(sparce_feature_size, 0U);

  std::vector<std::vector<uintptr_t>> batches;
  batches.reserve(minibatch_size);
  for (auto i = 0; i < minibatch_size; ++i) {
    batches.emplace_back(indices_vec(num_lookup, num_lookup + i));
  }

  for ([[maybe_unused]] auto _ : state) {
    for (auto &indices : batches) {
      engine.offset_transform_reduce_vec(&(*indices.begin()), &(*indices.end()),
                                         pnm::make_view(result), sls_op);
      benchmark::DoNotOptimize(result);
    }
  }
  auto v = std::accumulate(result.begin(), result.end(), 0U);
  benchmark::DoNotOptimize(v);
}

void minibatch_1024_numlookup_10(benchmark::State &state) {
  benchmark_core(1024, 10, state);
}
BENCHMARK(minibatch_1024_numlookup_10);

void minibatch_512_numlookup_20(benchmark::State &state) {
  benchmark_core(512, 20, state);
}
BENCHMARK(minibatch_512_numlookup_20);

void minibatch_256_numlookup_40(benchmark::State &state) {
  benchmark_core(256, 40, state);
}
BENCHMARK(minibatch_256_numlookup_40);

void minibatch_128_numlookup_80(benchmark::State &state) {
  benchmark_core(128, 80, state);
}
BENCHMARK(minibatch_128_numlookup_80);

void minibatch_64_numlookup_160(benchmark::State &state) {
  benchmark_core(64, 160, state);
}
BENCHMARK(minibatch_64_numlookup_160);

void minibatch_1024_numlookup_10_old(benchmark::State &state) {
  benchmark_core_old(1024, 10, state);
}
BENCHMARK(minibatch_1024_numlookup_10_old);

void minibatch_512_numlookup_20_old(benchmark::State &state) {
  benchmark_core_old(512, 20, state);
}
BENCHMARK(minibatch_512_numlookup_20_old);

void minibatch_256_numlookup_40_old(benchmark::State &state) {
  benchmark_core_old(256, 40, state);
}
BENCHMARK(minibatch_256_numlookup_40_old);

void minibatch_128_numlookup_80_old(benchmark::State &state) {
  benchmark_core_old(128, 80, state);
}
BENCHMARK(minibatch_128_numlookup_80_old);

void minibatch_64_numlookup_160_old(benchmark::State &state) {
  benchmark_core_old(64, 160, state);
}
BENCHMARK(minibatch_64_numlookup_160_old);

BENCHMARK_MAIN();

// NOLINTEND(misc-use-anonymous-namespace)
