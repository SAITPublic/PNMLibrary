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

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include <benchmark/benchmark.h>

#include <cstddef>
#include <vector>

void alloc_dealloc(benchmark::State &state) {
  const auto alloc_num = state.range(1);
  auto ctx = pnm::make_context(pnm::Device::Type::SLS);
  auto &alloc = ctx->allocator();
  std::vector<pnm::memory::DeviceRegion> dregion(alloc_num);

  // div_factor defines a freq. of hitting min desc value
  // on deallocation.
  // f.e. div_factor = 4 will generate the following deallocation
  // desc sequence: 4, 3, 2, 1 (hit min), 8, 7, 6, 5 (hit min), ...
  // such sequence is intended to test cached rb_tree as desc container
  // on kernel side.
  const auto div_factor = state.range(0);
  std::vector<size_t> indices;
  indices.reserve(alloc_num);

  for (auto i = 0; i < (alloc_num / div_factor); ++i) {
    for (auto j = div_factor; j-- > 0;) {
      auto idx = i * div_factor + j;
      indices.push_back(idx);
    }
  }

  static constexpr auto alloc_size = 4096;

  for ([[maybe_unused]] auto _ : state) {
    for (auto &reg : dregion) {
      reg = alloc->allocate(alloc_size);
    }

    for (auto &idx : indices) {
      alloc->deallocate(dregion[idx]);
    }
  }
}

BENCHMARK(alloc_dealloc)
    ->RangeMultiplier(2)
    ->Ranges({{2, 8}, {128, 128 << 4}})
    ->ArgNames({"div_factor", "alloc_num"})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();

// NOLINTEND(misc-use-anonymous-namespace)
