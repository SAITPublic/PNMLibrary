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

#include "pnmlib/common/properties.h"

#include "pnmlib/core/allocator.h"

#include <gtest/gtest.h>

#include <linux/sls_resources.h>

#include <array>
#include <stdexcept>

using namespace pnm::property;
using namespace pnm::memory::property;

TEST(Property, RankLocation) {
  static constexpr auto rank_num = 10;
  const CURegion loc{rank_num};
  ASSERT_EQ(loc.rank(), rank_num);

  const PropertiesList props(loc);
  ASSERT_TRUE(props.has_property<CURegion>());
  ASSERT_EQ(props.get_property<CURegion>().rank(), rank_num);
}

TEST(Property, AllocPolicy) {
  const std::array policies{SLS_ALLOC_SINGLE, SLS_ALLOC_AUTO,
                            SLS_ALLOC_DISTRIBUTE_ALL, SLS_ALLOC_REPLICATE_ALL};
  for (auto policy : policies) {
    const AllocPolicy prop_policy(policy);
    ASSERT_EQ(prop_policy.policy(), policy);

    const PropertiesList props(prop_policy);
    ASSERT_TRUE(props.has_property<AllocPolicy>());
    ASSERT_EQ(props.get_property<AllocPolicy>().policy(), policy);
  }
}

TEST(Property, PropertyChain) {
  static constexpr auto rank_id = 3;
  auto all_exist = [](const PropertiesList &props) {
    ASSERT_TRUE(props.has_property<CURegion>());
    ASSERT_TRUE(props.has_property<AllocPolicy>());

    ASSERT_EQ(props.get_property<CURegion>().rank(), rank_id);
    ASSERT_EQ(props.get_property<AllocPolicy>().policy(),
              SLS_ALLOC_DISTRIBUTE_ALL);
  };
  all_exist(CURegion(rank_id) | AllocPolicy(SLS_ALLOC_DISTRIBUTE_ALL));
  all_exist({CURegion(rank_id), AllocPolicy(SLS_ALLOC_DISTRIBUTE_ALL)});

  auto rank_exist = [](const PropertiesList &props) {
    ASSERT_TRUE(props.has_property<CURegion>());
    ASSERT_FALSE(props.has_property<AllocPolicy>());

    ASSERT_EQ(props.get_property<CURegion>().rank(), rank_id);

    EXPECT_THROW(props.get_property<AllocPolicy>().policy(), std::out_of_range);
  };
  rank_exist(CURegion(rank_id));
}
