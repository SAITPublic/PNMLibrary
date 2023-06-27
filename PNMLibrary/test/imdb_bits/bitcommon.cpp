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

#include "pnmlib/imdb/bit_common.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <utility>

TEST(BitOps, Alignment) {
  using namespace pnm::bitutils;
  ASSERT_EQ(align_size(1, 8), 8);
  ASSERT_EQ(align_size(12, 16), 16);
  ASSERT_EQ(align_size(16, 16), 16);
  ASSERT_EQ(align_size(17, 16), 32);

  ASSERT_EQ((aligned_storage_size<uint32_t>(15)), 2);
  ASSERT_EQ((aligned_storage_size<uint32_t>(15, 512)), 16);
  ASSERT_EQ((aligned_storage_size<uint32_t>(32 * 16, 512)), 16);
  ASSERT_EQ((aligned_storage_size<uint32_t>(32 * 17, 512)), 32);
  ASSERT_EQ((aligned_storage_size<uint32_t>(32 * 17, 256)), 24);
}

TEST(BitOps, BitPositionAndMask) {
  using namespace pnm::bitutils;
  ASSERT_EQ((in_storage_bit_position<uint32_t>(5)), (std::pair{0UL, 5UL}));
  ASSERT_EQ((in_storage_bit_position<uint32_t>(42)), (std::pair{1UL, 10UL}));
  ASSERT_EQ((in_storage_bit_position<uint8_t>(65)), (std::pair{8UL, 1UL}));
  ASSERT_EQ((in_storage_bit_position<uint8_t>(64)), (std::pair{8UL, 0UL}));

  ASSERT_EQ((in_storage_bit_position_masked<uint32_t>(5)),
            (std::pair{0UL, 1UL << 5UL}));
  ASSERT_EQ((in_storage_bit_position_masked<uint32_t>(42)),
            (std::pair{1UL, 1UL << 10UL}));
  ASSERT_EQ((in_storage_bit_position_masked<uint8_t>(65)),
            (std::pair{8UL, 1UL << 1UL}));
  ASSERT_EQ((in_storage_bit_position_masked<uint8_t>(64)),
            (std::pair{8UL, 1UL}));

  ASSERT_EQ((in_storage_bit_position_masked<uint8_t>(0)),
            (std::pair{0UL, 1UL}));
}

template <typename storage_t, typename access_t>
bool check_align_and_access(unsigned long ALIGN_BIT) {
  using namespace pnm::bitutils;
  static constexpr auto MAX_BIT = 4096;
  long element_index = 0;
  long bit_index = 0;
  long storage_size = 0;
  long align_value = 0;
  for (auto i = 0UL; i < MAX_BIT; ++i) {
    EXPECT_EQ(align_size(i, ALIGN_BIT), align_value);
    EXPECT_EQ((aligned_storage_size<storage_t>(i, ALIGN_BIT)), storage_size);

    if (i % ALIGN_BIT == 0) {
      align_value += ALIGN_BIT;
      storage_size = align_value / (8L * sizeof(storage_t));
    }

    auto [position, shift] = in_storage_bit_position<storage_t>(i);
    auto [alt_position, mask] = in_storage_bit_position_masked<storage_t>(i);

    EXPECT_EQ(position, element_index);
    EXPECT_EQ(shift, bit_index);
    EXPECT_EQ(mask, storage_t{1} << bit_index);

    bit_index = (bit_index + 1) % (sizeof(storage_t) * 8L);
    if (bit_index == 0) {
      ++element_index;
    }
  }
  return ::testing::Test::HasFailure();
}

TEST(BitOps, PositionAndAlignLarge) {
  ASSERT_FALSE((check_align_and_access<uint8_t, uint16_t>(128)))
      << "Fail test: <uint8_t, uint16_t>, align = 256 ";
  ASSERT_FALSE((check_align_and_access<uint16_t, uint32_t>(128)))
      << "Fail test: <uint16_t, uint32_t>, align = 256 ";
  ASSERT_FALSE((check_align_and_access<uint32_t, uint64_t>(128)))
      << "Fail test: <uint32_t, uint64_t>, align = 256 ";

  ASSERT_FALSE((check_align_and_access<uint8_t, uint16_t>(256)))
      << "Fail test: <uint8_t, uint16_t>, align = 256 ";
  ASSERT_FALSE((check_align_and_access<uint16_t, uint32_t>(256)))
      << "Fail test: <uint16_t, uint32_t>, align = 256 ";
  ASSERT_FALSE((check_align_and_access<uint32_t, uint64_t>(256)))
      << "Fail test: <uint32_t, uint64_t>, align = 256 ";

  ASSERT_FALSE((check_align_and_access<uint8_t, uint16_t>(512)))
      << "Fail test: <uint8_t, uint16_t>, align = 256 ";
  ASSERT_FALSE((check_align_and_access<uint16_t, uint32_t>(512)))
      << "Fail test: <uint16_t, uint32_t>, align = 256 ";
  ASSERT_FALSE((check_align_and_access<uint32_t, uint64_t>(512)))
      << "Fail test: <uint32_t, uint64_t>, align = 256 ";
}
