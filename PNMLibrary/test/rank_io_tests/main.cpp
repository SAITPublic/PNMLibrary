/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 */

#include "sls/operation/inst_generator.h"

#include "core/device/sls/rank_address.h"

#include "common/compiler_internal.h"
#include "common/topology_constants.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

namespace {
using pnm::device::topo;

ALWAYS_INLINE
uintptr_t golden_interleaved_phys_offset(uintptr_t rank_offset,
                                         bool is_second_rank) {
  // Calculating the address below is according to the Rank Interleaving rule:
  //   - interleaving every 128B (rank)
  //   - pattern switches every 128 KB

  // Offset within chunk.
  const auto chunk_offset = rank_offset % topo().RankInterleavingSize;

  // Physical offset base: total size of whole chunks * num ranks per cs.
  auto offset =
      (rank_offset - chunk_offset) * (topo().NumOfRanks / topo().NumOfCS);

  // Pattern switches every 128 KB.
  const auto pattern_count = offset / topo().ChannelInterleavingSize;

  // Correction for the first rank (even pattern count): 0 bytes.
  // Correction for the second rank (odd pattern count): ±128 bytes.
  const uintptr_t rank_interleaving_size = topo().RankInterleavingSize;
  if (is_second_rank) {
    offset -= (pattern_count % 2) * rank_interleaving_size;
  } else {
    offset += (pattern_count % 2) * rank_interleaving_size;
  }

  // Add the chunk offset back.
  return offset + chunk_offset;
}
} // namespace

TEST(RankIO, InterleavedCopyAligned) {
  static constexpr auto NBYTES = (4486ULL << 10);
  static constexpr auto N =
      NBYTES / sizeof(uint64_t); // fill 4486Kb with uint64_t values

  std::vector<uint64_t> data(N);
  std::iota(data.begin(), data.end(), 0);

  for (auto HA : {0, 1}) {
    auto storage = std::make_unique<uint8_t[]>(2 * NBYTES);
    std::fill_n(storage.get(), 2 * NBYTES, 0);

    const pnm::sls::device::InterleavedPointer iptr(storage.get(), HA);
    ASSERT_NO_THROW(pnm::sls::device::memcpy_interleaved(
        data.data(), data.size() * sizeof(uint64_t), iptr));

    for (auto i = 0UL; i < NBYTES; i += sizeof(uint64_t)) {
      auto *golden = storage.get() + golden_interleaved_phys_offset(i, HA) +
                     HA * topo().RankInterleavingSize;
      ASSERT_EQ(*reinterpret_cast<uint64_t *>(golden),
                data[i / sizeof(uint64_t)]);
      *reinterpret_cast<uint64_t *>(golden) *= 2;
    }

    std::vector<uint64_t> readed(data.size(), 0);
    ASSERT_NO_THROW(pnm::sls::device::memcpy_interleaved(
        iptr, readed.size() * sizeof(uint64_t), readed.data()));

    std::transform(readed.begin(), readed.end(), readed.begin(),
                   [](auto v) { return v / 2; });
    ASSERT_EQ(data, readed);
  }
}

TEST(RankIO, InterleavedCopyUnAligned) {
  static constexpr auto NBYTES = (4486ULL << 10);
  static constexpr auto N =
      NBYTES / sizeof(uint64_t); // fill 4486Kb with uint64_t values

  // We shift the ipointer by 13*sizeof(uint64_t) to verify unaligned copy
  static constexpr auto UNALIGNED_SHIFT = 13;

  std::vector<uint64_t> data(N - UNALIGNED_SHIFT);
  std::iota(data.begin(), data.end(), 0);

  for (auto HA : {0, 1}) {
    auto storage = std::make_unique<uint8_t[]>(2 * NBYTES);
    std::fill_n(storage.get(), 2 * NBYTES, 0);

    pnm::sls::device::InterleavedPointer iptr(storage.get(), HA);

    iptr += UNALIGNED_SHIFT * sizeof(uint64_t);

    ASSERT_NO_THROW(pnm::sls::device::memcpy_interleaved(
        data.data(), data.size() * sizeof(uint64_t), iptr));

    for (auto i = UNALIGNED_SHIFT * sizeof(uint64_t); i < NBYTES;
         i += sizeof(uint64_t)) {
      auto *golden = storage.get() + golden_interleaved_phys_offset(i, HA) +
                     HA * topo().RankInterleavingSize;
      ASSERT_EQ(*reinterpret_cast<uint64_t *>(golden),
                data[i / sizeof(uint64_t) - UNALIGNED_SHIFT]);
      *reinterpret_cast<uint64_t *>(golden) *= 2;
    }

    std::vector<uint64_t> readed(data.size(), 0);
    ASSERT_NO_THROW(pnm::sls::device::memcpy_interleaved(
        iptr, readed.size() * sizeof(uint64_t), readed.data()));

    std::transform(readed.begin(), readed.end(), readed.begin(),
                   [](auto v) { return v / 2; });
    ASSERT_EQ(data, readed);
  }
}

TEST(InterleavedAddress, VerifyTransformation) {
  static constexpr auto MAX_VALUE = 16ULL << 30; // 16GB max
  static constexpr auto NCHECKS = 10'000'000;

  std::mt19937_64 rgen(std::random_device{}());
  std::uniform_int_distribution<uintptr_t> rand_addr_gen(0, MAX_VALUE);

  auto debug_addr_components_str = [](uintptr_t rank_addr) {
    pnm::sls::RankedInstGenerator::DecodedInst instr;
    pnm::sls::RankedInstGenerator::decode_address_to_instruction(rank_addr,
                                                                 instr);
    return fmt::format("Col: {}, Row: {}, BA: {}, BG: {}\n", instr.column,
                       instr.row, instr.bank, instr.bankgroup);
  };

  static constexpr auto ALIGN = ~((1ULL << 3) - 1);
  for (auto i = 0UL; i < NCHECKS; ++i) {
    auto rank_addr = rand_addr_gen(rgen) & ALIGN;
    for (auto HA : {0, 1}) {
      uint64_t iaddress = pnm::sls::device::interleaved_address(rank_addr, HA);

      static constexpr auto ISIZE = 128;
      uint64_t golden =
          ::golden_interleaved_phys_offset(rank_addr, HA) + ISIZE * HA;

      ASSERT_EQ(iaddress, golden) << fmt::format(
          "HA [{0:}]: address "
          "mismatch.\nRanked:\t{1:}({1:b}),\nInterleaved:\t{2:}({2:b}),"
          "\nGolden:\t{3:}({3:b}).\nAddress components: {4}",
          HA, rank_addr, iaddress, golden,
          debug_addr_components_str(rank_addr));
    }
  }
}

TEST(InterleavedPointer, VerifyTransformation) {
  static constexpr auto NCHECKS = 10'000'000;

  for (auto HA : {0, 1}) {
    uint32_t dummy_var = 42;

    auto golden_ptr = [&dummy_var, HA](uintptr_t rank_offset) {
      return reinterpret_cast<uint32_t *>(
          reinterpret_cast<uint8_t *>(&dummy_var) +
          pnm::sls::device::interleaved_address(rank_offset, HA));
    };

    const pnm::sls::device::InterleavedPointer iptr(&dummy_var, HA);
    pnm::sls::device::InterleavedPointer iptr_inc = iptr;
    pnm::sls::device::InterleavedPointer iptr_dec = iptr + NCHECKS;

    for (auto i = 0UL; i < NCHECKS; ++i) {
      ASSERT_EQ((iptr + i).get(), golden_ptr(i));

      ++iptr_inc;
      const auto golden_inc = iptr + i + 1;
      ASSERT_EQ(golden_inc, iptr_inc);

      --iptr_dec;
      const auto golden_dec = iptr + NCHECKS - i - 1;
      ASSERT_EQ(golden_dec, iptr_dec);
    }
  }
}
