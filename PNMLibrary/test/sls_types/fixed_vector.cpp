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
#include "common/sls_types.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <initializer_list>
#include <iterator>

template <typename FwIt1, typename FwIt2>
void check_ranges(FwIt1 begin1, FwIt1 end1, FwIt2 begin2, FwIt2 end2) {
  EXPECT_TRUE(std::equal(begin1, end1, begin2, end2));
}

template <typename R1, typename R2>
void check_ranges(const R1 &test_range, const R2 &golden_range) {
  check_ranges(std::begin(test_range), std::end(test_range),
               std::begin(golden_range), std::end(golden_range));
}

template <typename R1, typename T>
void check_ranges(const R1 &test_range,
                  const std::initializer_list<T> &golden_range) {
  check_ranges(std::begin(test_range), std::end(test_range),
               std::begin(golden_range), std::end(golden_range));
}

TEST(FixedVector, BaseOperation) {
  pnm::types::FixedVector<int, 5> a{1, 2, 3, 4, 5};
  check_ranges(a, {1, 2, 3, 4, 5});
  ASSERT_EQ(a.size(), 5);

  pnm::types::FixedVector<int, 5> b{5, 4, 3, 4, 5};
  auto c = a + b;
  check_ranges(c, {6, 6, 6, 8, 10});

  c = a - b;
  check_ranges(c, {-4, -2, 0, 0, 0});

  a += c;
  check_ranges(a, {-3, 0, 3, 4, 5});

  b -= c;
  check_ranges(b, {9, 6, 3, 4, 5});
}

TEST(FixedVector, VectorCompare) {
  const pnm::types::FixedVector<int, 4> lhs{1, 2, 3, 4};
  const pnm::types::FixedVector<int, 4> rhs_equal{1, 2, 3, 4};
  const pnm::types::FixedVector<int, 4> rhs_nequal{4, 3, 2, 1};

  ASSERT_TRUE(lhs == rhs_equal);
  ASSERT_TRUE(lhs != rhs_nequal);
}
