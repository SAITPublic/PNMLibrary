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

#include "allocator.h"

#include "common/compiler_internal.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <array>

inline constexpr std::array stress_modes{StressMode::multithreadedCapacity};

INSTANTIATE_TEST_SUITE_P(Stress, Allocator,
                         testing::Combine(testing::ValuesIn(stress_modes),
                                          testing::ValuesIn(alloc_pref)),
                         print_test_name);

TEST_P(Allocator, Tests) {
  if constexpr (TSAN) {
    GTEST_SKIP() << "This tests allocate all SLS memory and TSan increase "
                    "memory consumption explicitly to x8 times"
                 << '\n';
  }

  const auto [mode, distrib] = GetParam();
  UnitTest test({mode, distrib});
  test.run();
}
