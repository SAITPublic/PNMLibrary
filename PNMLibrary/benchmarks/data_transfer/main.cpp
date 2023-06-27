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

#include "common/mapped_file.h"

#include "pnmlib/imdb/libimdb.h"

#include "pnmlib/common/misc_utils.h"

#include <benchmark/benchmark.h>

#include <fmt/core.h>

#include <emmintrin.h>
#include <immintrin.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <random>

constexpr auto ALIGN = 64;
constexpr auto BENCH_THREADS = 8;

constexpr auto DEV_MEM_PATH = "/dev/mem";
constexpr auto DEV_MEM_OFFSET = 0x1080000000UL;

class RamAlloc {
public:
  explicit RamAlloc([[maybe_unused]] int id, uint64_t size)
      : data_(static_cast<uint8_t *>(std::aligned_alloc(ALIGN, size))) {}

  ~RamAlloc() { std::free(data_); }

  uint8_t *data() { return data_; }

private:
  uint8_t *data_;
};

// NOTE: All allocated in rank 0

class DeviceWCAlloc {
public:
  explicit DeviceWCAlloc(int id, uint64_t size)
      : map_(IMDB_DEVMEMORY_PATH, size, "device memory", id * size) {}

  uint8_t *data() { return static_cast<uint8_t *>(map_.data()); }

private:
  pnm::utils::MappedFile map_;
};

class DeviceUCAlloc {
public:
  explicit DeviceUCAlloc(int id, uint64_t size)
      : map_(DEV_MEM_PATH, size, "device memory", DEV_MEM_OFFSET + id * size) {}

  uint8_t *data() { return static_cast<uint8_t *>(map_.data()); }

private:
  pnm::utils::MappedFile map_;
};

void add_mismatches(benchmark::State &state, const uint8_t *golden,
                    const uint8_t *got, uint64_t size) {
  uint64_t mismatches = 0;

  for (uint64_t i = 0; i < size; ++i) {
    if (golden[i] != got[i]) {
      ++mismatches;
    }
  }

  state.counters["Mismatches"] = benchmark::Counter(
      static_cast<double>(mismatches) / size, benchmark::Counter::kAvgThreads,
      benchmark::Counter::OneK::kIs1000);
}

void add_bandwidth(benchmark::State &state, uint64_t size) {
  state.counters["Bandwidth"] =
      benchmark::Counter(state.iterations() * size, benchmark::Counter::kIsRate,
                         benchmark::Counter::OneK::kIs1024);
}

// NOTE: For RAM we have a 1GB buffer because the buffer must be much bigger
// than CPU caches. Otherwise the results are unreliable. And for device we have
// a pretty small 2MB buffer because it is much slower in general

struct RamParams {
  static constexpr auto name = "ram";
  static constexpr uint64_t size = 1UL << 30;
  using src_alloc = RamAlloc;
  using dst_alloc = RamAlloc;
};

struct ToDeviceWCParams {
  static constexpr auto name = "to_device_wc";
  static constexpr uint64_t size = 2UL << 20;
  using src_alloc = RamAlloc;
  using dst_alloc = DeviceWCAlloc;
};

struct ToDeviceUCParams {
  static constexpr auto name = "to_device_uc";
  static constexpr uint64_t size = 2UL << 20;
  using src_alloc = RamAlloc;
  using dst_alloc = DeviceUCAlloc;
};

struct FromDeviceWCParams {
  static constexpr auto name = "from_device_wc";
  static constexpr uint64_t size = 2UL << 20;
  using src_alloc = DeviceWCAlloc;
  using dst_alloc = RamAlloc;
};

struct FromDeviceUCParams {
  static constexpr auto name = "from_device_uc";
  static constexpr uint64_t size = 2UL << 20;
  using src_alloc = DeviceUCAlloc;
  using dst_alloc = RamAlloc;
};

struct AvxParams {
  static constexpr auto name = "avx";
  static void copy(const uint8_t *src, uint8_t *dst, uint64_t size) {
    const auto *vsrc = reinterpret_cast<const __m256i *>(src);
    auto *vdst = reinterpret_cast<__m256i *>(dst);

    for (uint64_t i = 0; i < (size / 32) - 1; i += 2) {
      const __m256i a = _mm256_load_si256(&vsrc[i]);
      const __m256i b = _mm256_load_si256(&vsrc[i + 1]);
      _mm256_store_si256(&vdst[i], a);
      _mm256_store_si256(&vdst[i + 1], b);
    }
  }
};

struct AvxStreamParams {
  static constexpr auto name = "avx_stream";
  static void copy(const uint8_t *src, uint8_t *dst, uint64_t size) {
    const auto *vsrc = reinterpret_cast<const __m256i *>(src);
    auto *vdst = reinterpret_cast<__m256i *>(dst);

    for (uint64_t i = 0; i < (size / 32) - 1; i += 2) {
      const __m256i a = _mm256_stream_load_si256(&vsrc[i]);
      const __m256i b = _mm256_stream_load_si256(&vsrc[i + 1]);
      _mm256_stream_si256(&vdst[i], a);
      _mm256_stream_si256(&vdst[i + 1], b);
    }
  }
};

struct AvxFlushParams {
  static constexpr auto name = "avx_flush";
  static void copy(const uint8_t *src, uint8_t *dst, uint64_t size) {
    const auto *vsrc = reinterpret_cast<const __m256i *>(src);
    auto *vdst = reinterpret_cast<__m256i *>(dst);

    for (uint64_t i = 0; i < (size / 32) - 1; i += 2) {
      const __m256i a = _mm256_stream_load_si256(&vsrc[i]);
      const __m256i b = _mm256_stream_load_si256(&vsrc[i + 1]);
      _mm256_stream_si256(&vdst[i], a);
      _mm256_stream_si256(&vdst[i + 1], b);
      // One clflush spans 64 bytes
      _mm_clflush(&vdst[i]);
    }
  }
};

struct AvxOnePerIterParams {
  static constexpr auto name = "avx_one_per_iter";
  static void copy(const uint8_t *src, uint8_t *dst, uint64_t size) {
    const auto *vsrc = reinterpret_cast<const __m256i *>(src);
    auto *vdst = reinterpret_cast<__m256i *>(dst);

    for (uint64_t i = 0; i < size / 32; i += 1) {
      const __m256i a = _mm256_stream_load_si256(&vsrc[i]);
      _mm256_stream_si256(&vdst[i], a);
    }
  }
};

struct StdParams {
  static constexpr auto name = "std";
  static void copy(const uint8_t *src, uint8_t *dst, uint64_t size) {
    std::copy(src, src + size, dst);
  }
};

using pnm::utils::tuple_product_t;
using pnm::utils::TypeTuple;

using BenchTypes =
    tuple_product_t<TypeTuple<RamParams, ToDeviceWCParams, ToDeviceUCParams,
                              FromDeviceWCParams, FromDeviceUCParams>,
                    TypeTuple<AvxParams, AvxStreamParams, AvxFlushParams,
                              AvxOnePerIterParams, StdParams>>;

void setup_bench(benchmark::internal::Benchmark *benchmark) {
  benchmark->ThreadRange(1, BENCH_THREADS);
  benchmark->UseRealTime();
  benchmark->Unit(benchmark::kMillisecond);
}

template <typename Tuple> struct Benchmark {};

template <typename DataParams, typename CopyParams>
struct Benchmark<TypeTuple<DataParams, CopyParams>> {
  using src_alloc = typename DataParams::src_alloc;
  using dst_alloc = typename DataParams::dst_alloc;
  using copy_func_type = decltype(CopyParams::copy);
  static constexpr auto size = DataParams::size;

  static void setup() {
    // Must be static because gbench expects a c-string, not a normal string
    static auto name = fmt::format("{}_{}", DataParams::name, CopyParams::name);
    benchmark::RegisterBenchmark(name, [](benchmark::State &state) {
      benchmark(state, src_alloc{state.thread_index(), size},
                dst_alloc{state.thread_index(), size}, CopyParams::copy);
    })->Apply(setup_bench);
  }

  static void benchmark(benchmark::State &state, src_alloc src_alloc,
                        dst_alloc dst_alloc, copy_func_type copy_func) {
    uint8_t *src = src_alloc.data();

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(
        0, std::numeric_limits<uint8_t>::max());
    std::memset(src, dist(rng), size);

    uint8_t *dst = dst_alloc.data();

    assert((uintptr_t)src % ALIGN == 0);
    assert((uintptr_t)dst % ALIGN == 0);
    assert((uintptr_t)size % ALIGN == 0);

    for ([[maybe_unused]] auto _ : state) {
      copy_func(src, dst, size);
      benchmark::DoNotOptimize(src);
      benchmark::DoNotOptimize(dst);
      benchmark::ClobberMemory();
    }

    add_mismatches(state, src, dst, size);
    add_bandwidth(state, size);
  }
};

template <typename... ParamTuples>
void setup_all_benchmarks(TypeTuple<ParamTuples...> /*unused*/) {
  (Benchmark<ParamTuples>::setup(), ...);
}

int main(int argc, char **argv) {
  setup_all_benchmarks(BenchTypes{});
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}

// NOLINTEND(misc-use-anonymous-namespace)
