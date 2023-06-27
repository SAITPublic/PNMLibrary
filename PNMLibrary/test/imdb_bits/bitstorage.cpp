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

#include "pnmlib/imdb/aligned_allocator.h"
#include "pnmlib/imdb/bit_common.h"
#include "pnmlib/imdb/bit_containers.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <list>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

TEST(AlignedAllocator, ConstructAllocateDeallocate) {
  static constexpr auto alignment = 32;
  const pnm::utils::aligned_allocator<uint32_t, alignment> alloc;
  static constexpr auto alloc_tries = 1000;
  for (auto i = 0; i < alloc_tries; ++i) {
    auto *object = alloc.allocate(i);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(object) % alignment, 0);
    alloc.deallocate(object, i);
  }

  struct Probe {
    double a;
    std::string b;
    std::array<int, 4> c;
    Probe(double a, std::string b, std::array<int, 4> c)
        : a{a}, b{std::move(b)}, c{c} {}
  };

  const pnm::utils::aligned_allocator<Probe, alignment> palloc;
  auto *object = palloc.allocate(1);
  ASSERT_EQ(reinterpret_cast<uintptr_t>(object) % alignment, 0);

  static constexpr auto pi = 3.14;
  static constexpr std::string_view msg = "Hello sweaty";
  static constexpr std::array probe_array{1, 2, 3, 4};
  palloc.construct(object, pi, msg.cbegin(), probe_array);

  ASSERT_EQ(object->a, pi);
  ASSERT_EQ(object->b, msg);
  ASSERT_EQ(object->c, probe_array);

  palloc.destroy(object);
  palloc.deallocate(object, 1);

  // Mostly compilation with allocation/deallocation check. We can't easily
  // check the alignment of list elements due to complex internal structure of
  // std::list
  std::list<uint64_t, pnm::utils::aligned_allocator<uint64_t, alignment>>
      aligned_list(probe_array.begin(), probe_array.end());
  aligned_list.emplace_back(42);
  aligned_list.emplace_back(24);
}

TEST(CompressedVector, ConstructionReadWrite) {
  const pnm::containers::compressed_vector cv1;
  ASSERT_EQ(cv1.size(), 0UL);
  ASSERT_EQ(cv1.capacity(), 0UL);
  ASSERT_EQ(reinterpret_cast<uintptr_t>(cv1.data()) %
                pnm::containers::container_alignment,
            0);

  static constexpr auto BIT_SIZE = 5;

  const pnm::containers::compressed_vector cv2(BIT_SIZE, 20);
  ASSERT_EQ(reinterpret_cast<uintptr_t>(cv2.data()) %
                pnm::containers::container_alignment,
            0);

  ASSERT_EQ(cv2.size(), 20);

  static constexpr auto container_align = 512;
  ASSERT_EQ(cv2.capacity(),
            pnm::bitutils::aligned_storage_size<uint32_t>(
                BIT_SIZE * 20 + container_align, container_align) *
                8 * sizeof(uint32_t) / BIT_SIZE);

  std::vector value{4, 5, 6, 11};
  const pnm::containers::compressed_vector<uint8_t> cv3(BIT_SIZE, value.begin(),
                                                        value.end());
  ASSERT_EQ(reinterpret_cast<uintptr_t>(cv3.data()) %
                pnm::containers::container_alignment,
            0);
  ASSERT_EQ(cv3.size(), value.size());
  ASSERT_TRUE(std::equal(cv3.begin(), cv3.end(), value.begin()));

  static constexpr auto ilist = {5, 3, 22, 31, 14};
  pnm::containers::compressed_vector<uint8_t> cv4(BIT_SIZE, ilist);
  ASSERT_EQ(reinterpret_cast<uintptr_t>(cv4.data()) %
                pnm::containers::container_alignment,
            0);
  ASSERT_EQ(cv4.size(), 5);
  ASSERT_TRUE(std::equal(cv4.begin(), cv4.end(), ilist.begin()));

  const pnm::containers::compressed_vector<uint8_t> mcv = std::move(cv4);
  ASSERT_EQ(reinterpret_cast<uintptr_t>(mcv.data()) %
                pnm::containers::container_alignment,
            0);
  ASSERT_EQ(mcv.size(), 5);
  ASSERT_TRUE(std::equal(mcv.begin(), mcv.end(), ilist.begin()));
}

TEST(CompressedVector, FromRaw) {
  static constexpr auto BIT_SIZE = 8;

  std::vector value{1, 3, 4, 5, 8, 100};
  const pnm::containers::compressed_vector<uint8_t> vector(
      BIT_SIZE, value.begin(), value.end());

  ASSERT_EQ(vector.size(), value.size());
  ASSERT_TRUE(std::equal(vector.begin(), vector.end(), value.begin()));

  const auto from_raw = pnm::containers::compressed_view_from_raw(
      BIT_SIZE, vector.data(), vector.size());

  ASSERT_TRUE(std::equal(vector.begin(), vector.end(), from_raw.begin()));
}

TEST(CompressedVector, LargeTest) {
  static constexpr auto BIT_SIZE = 14;
  static constexpr auto N = 100'000;

  std::vector<uint32_t> golden(N);
  pnm::containers::compressed_vector<uint32_t> v(BIT_SIZE, N);
  ASSERT_EQ(reinterpret_cast<uintptr_t>(v.data()) %
                pnm::containers::container_alignment,
            0);

  std::mt19937_64 eng(std::random_device{}());
  std::uniform_int_distribution<uint32_t> uni(0, (1U << BIT_SIZE) - 1);
  for (auto i = 0; i < N; ++i) {
    v[i] = golden[i] = uni(eng);
  }

  uni = std::uniform_int_distribution<uint32_t>(0, N - 1);
  static constexpr auto request = 100'000;
  for (auto i = 0; i < request; ++i) {
    const auto index = uni(eng);
    ASSERT_EQ(static_cast<uint32_t>(v[index]), golden[index]);
  }
}

TEST(BitVector, ConstructionWriteRead) {
  static constexpr std::string_view text =
      "This approach lets you release the most pressure possible in 30 "
      "minutes";
  std::array<char, 127> char_map{};
  pnm::containers::bit_vector bit_map(127);
  ASSERT_EQ(reinterpret_cast<uintptr_t>(bit_map.data()) %
                pnm::containers::container_alignment,
            0);
  for (const unsigned char c : text) {
    char_map[c] = 1;
    bit_map.insert(c);
  }

  const pnm::containers::bit_vector bit_map_range_constr(text.begin(),
                                                         text.end());
  ASSERT_EQ(reinterpret_cast<uintptr_t>(bit_map_range_constr.data()) %
                pnm::containers::container_alignment,
            0);

  for (auto i = 0UL; i < char_map.size(); ++i) {
    ASSERT_EQ(char_map[i], bit_map.contains(i));
  }

  for (auto v : bit_map) {
    char_map[v] = 0;
  }

  ASSERT_EQ(std::find(char_map.begin(), char_map.end(), 1), char_map.end());

  ASSERT_TRUE(std::equal(bit_map_range_constr.begin(),
                         bit_map_range_constr.end(), bit_map.begin()));

  const auto ilist = {1, 45, 5, 7, 90, 45, 22};
  const pnm::containers::bit_vector bit_map_initlist(ilist);
  ASSERT_EQ(reinterpret_cast<uintptr_t>(bit_map_initlist.data()) %
                pnm::containers::container_alignment,
            0);
  ASSERT_EQ(bit_map_initlist.size(), 91);

  ASSERT_TRUE(bit_map_initlist.contains(45));
  ASSERT_TRUE(bit_map_initlist.contains(90));
  ASSERT_FALSE(bit_map_initlist.contains(18));
  ASSERT_FALSE(bit_map_initlist.contains(21));
  ASSERT_FALSE(bit_map_initlist.contains(23));

  auto golden = std::set(ilist);
  ASSERT_TRUE(std::equal(bit_map_initlist.begin(), bit_map_initlist.end(),
                         golden.begin()));
}

TEST(BitVector, FromRaw) {
  static constexpr std::string_view text =
      "What did you just say about me Ill have you know I graduated top of "
      "my class in the Navy Seals and Ive been involved in numerous secret "
      "raids on Al Quaeda and I have over 300 confirmed kills";
  pnm::containers::bit_vector bit_map(127);

  for (const unsigned char c : text) {
    bit_map.insert(c);
  }

  const auto from_raw =
      pnm::containers::bit_view_from_raw(bit_map.data(), bit_map.size());

  ASSERT_TRUE(std::equal(bit_map.begin(), bit_map.end(), from_raw.begin()));
}
