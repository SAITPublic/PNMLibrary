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

#include "core/device/imdb/base.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include <benchmark/benchmark.h>

#include <random>
#include <vector>

constexpr auto SEED = 42U;

void alloc_dealloc_fixed(benchmark::State &state) {
  const auto size_multiplier = state.range(0);

  std::vector<pnm::memory::DeviceRegion> regions;

  auto context = pnm::make_context(pnm::Device::Type::IMDB_CXL);
  auto &allocator = context->allocator();
  const auto &device = *context->device()->as<pnm::imdb::device::BaseDevice>();

  const auto region_size = device.alignment() * size_multiplier;
  const auto max_regions =
      device.memory_size() / (region_size * state.threads());

  std::default_random_engine generator(SEED + state.thread_index());
  std::bernoulli_distribution distribution(0.5);

  for ([[maybe_unused]] auto _ : state) {
    if (regions.size() < max_regions && distribution(generator)) {
      regions.emplace_back(allocator->allocate(region_size));
    } else {
      if (!regions.empty()) {
        allocator->deallocate(regions.back());
        regions.pop_back();
      }
    }
  }

  for (const auto &region : regions) {
    allocator->deallocate(region);
  }
}

BENCHMARK(alloc_dealloc_fixed)
    ->Range(1, 128)
    ->ArgName("size_multiplier")
    ->ThreadRange(1, 8);

// [TODO: @e-kutovoi] Non-multiple of 2 sizes
// [TODO: @e-kutovoi] Random sizes
// [TODO: @e-kutovoi] Small and big sizes together

BENCHMARK_MAIN();

// NOLINTEND(misc-use-anonymous-namespace)
