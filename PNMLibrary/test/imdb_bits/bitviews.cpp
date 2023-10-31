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
#include "pnmlib/imdb/bit_iterator.h"
#include "pnmlib/imdb/bit_views.h"

#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <fmt/core.h>
#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string_view>
#include <vector>

TEST(BitViewIterator, CreateAdvance) {
  struct iota_skipped {
    auto begin() const {
      return pnm::views::bit_iterator<size_t, iota_skipped>(*this, first);
    }
    auto end() const {
      return pnm::views::bit_iterator<size_t, iota_skipped>(*this, last);
    }

    size_t it_helper_value(uint64_t index) const { return index * scale; }
    uint64_t it_helper_next(uint64_t index) const {
      do {
        ++index;
      } while (index < last && index % div);

      return index;
    }

    size_t first, last;
    size_t scale;
    size_t div;
  };

  iota_skipped const test{0, 30, 2, 5};
  const std::vector golden{0, 10, 20, 30, 40, 50};

  fmt::print("{}\n", fmt::join(test, " "));

  ASSERT_TRUE(std::equal(test.begin(), test.end(), golden.begin()));

  iota_skipped const empty{0, 0, 2, 5};
  ASSERT_EQ(std::distance(empty.begin(), empty.end()), 0);
}

TEST(BitCompressedView, TrivialAccess) {
  static constexpr auto VALUE_BITS = 3;
  static constexpr auto VALUE_COUNT = 5;
  static constexpr auto N =
      pnm::bitutils::aligned_storage_size<uint8_t>(VALUE_COUNT * VALUE_BITS);
  const std::unique_ptr<uint8_t[]> memory(new uint8_t[N]);
  std::fill_n(memory.get(), N, 0);

  static constexpr std::array golden{0b001, 0b011, 0b111, 0b110, 0b101};

  *(reinterpret_cast<uint16_t *>(memory.get())) = 0b0'101'110'111'011'001;

  const pnm::views::bit_compressed vw(
      VALUE_BITS, VALUE_COUNT,
      pnm::views::common{memory.get(), memory.get() + N});

  auto gid = 0;
  for (auto e : vw) {
    ASSERT_EQ(golden[gid++], e)
        << fmt::format("Value mismatch for {} index", gid - 1);
  }

  for (auto i = 0UL; i < vw.size(); ++i) {
    ASSERT_EQ(golden[i], vw[i]) << fmt::format(
        "Value mismatch for {} index in operator[] test.", i - 1);
  }
}

TEST(BitCompressedView, ReadWriteAccess) {
  static constexpr std::string_view test_message{"Hello my bit friend"};
  static constexpr std::array<uint64_t, 4> golden{
      0b00100000111100111011010100000110'11111101100110110011001011001000,
      0b00110111011001011101001111001011'00110010000011101001101001110001,
      0b00000000000000000000000000000000'00000000000000000000000000011001,
      0b00000000000000000000000000000000'00000000000000000000000000000000};

  static constexpr auto VALUE_BITS = 7;
  static constexpr auto VALUE_COUNT = test_message.size();
  static constexpr auto N =
      pnm::bitutils::aligned_storage_size<uint64_t>(VALUE_COUNT * VALUE_BITS);
  const std::unique_ptr<uint64_t[]> memory(new uint64_t[N]);
  std::fill_n(memory.get(), N, 0);

  pnm::views::bit_compressed vw(
      VALUE_BITS, VALUE_COUNT,
      pnm::views::common{memory.get(), memory.get() + N});

  auto gid = 0;
  for (auto c : test_message) {
    if (gid % 2 == 0) {
      vw.set_value(gid, c);
    } else {
      vw[gid] = c;
    }
    ++gid;
  }

  for (auto e : vw) {
    fmt::print("{:c}", e);
  }

  ASSERT_TRUE(std::equal(golden.begin(), golden.end(),
                         pnm::views::make_view(memory.get(), N).begin()));
}

TEST(BitVectorView, CreateReadWrite) {
  std::array<uint64_t, 2> mem{0, 0};
  static constexpr auto MAX_V = 90;
  pnm::views::bit_vector_view bv(MAX_V, mem.begin(), mem.end());

  ASSERT_EQ(bv.size(), MAX_V);
  ASSERT_TRUE(std::distance(bv.begin(), bv.end()) == 0);

  mem[0] = 0b01001; // 0 and 3bit is on
  ASSERT_TRUE(bv.contains(0));
  ASSERT_TRUE(bv.contains(3));
  mem[1] = 0b10001; // 64 and 68 bits is on
  ASSERT_TRUE(bv.contains(64));
  ASSERT_TRUE(bv.contains(68));

  const std::vector golden_1{0, 3, 64, 68};
  ASSERT_TRUE(std::equal(bv.begin(), bv.end(), golden_1.begin()));

  bv.erase(3);
  ASSERT_FALSE(bv.contains(3));

  const std::vector golden_2{0, 64, 68};
  ASSERT_TRUE(std::equal(bv.begin(), bv.end(), golden_2.begin()));

  bv.clear();
  ASSERT_TRUE(std::distance(bv.begin(), bv.end()) == 0);
}
