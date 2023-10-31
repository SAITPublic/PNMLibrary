#include "core/device/sls/utils/inst_generator.h"
#include "core/device/sls/utils/rank_address.h"

#include "common/topology_constants.h"
#include "hw/pnm_addr_constants.h"
#include "hw/pnm_hw_instr_constants.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <random>

TEST(RankAddr, EncodeDecodeRankAddr) {
  /* current rank_address bits usage: [3:33] */
  constexpr uint64_t rank_addr_max = (1ULL << 33ULL);
  constexpr uint64_t rank_addr_step = (1ULL << 3ULL);
  for (uint64_t ra = 0x00; ra < rank_addr_max; ra += rank_addr_step) {
    pnm::sls::RankedInstGenerator::DecodedInst instr = {};

    pnm::sls::RankedInstGenerator::decode_address_to_instruction(ra, instr);
    const uint64_t ra_res =
        pnm::sls::RankedInstGenerator::encode_address_from_instruction(instr);

    ASSERT_TRUE(ra == ra_res);
  }
}

TEST(ChannelAddress, EncodeDecodeRankAddr) {
  /* current rank_address bits usage: [3:34] */
  static constexpr uint64_t channel_addr_max = (1ULL << 34ULL);
  static constexpr uint64_t channel_addr_min = (1ULL << 3ULL);

  std::mt19937_64 engine(std::random_device{}());
  std::uniform_int_distribution<uint64_t> address_gen{channel_addr_min,
                                                      channel_addr_max};

  static constexpr auto checks_count = 1'000'000;

  for (uint64_t i = 0; i < checks_count; ++i) {
    pnm::sls::ChannelInstGenerator::DecodedInst instr = {};

    const auto address = address_gen(engine) & (~(0b111ULL));

    pnm::sls::ChannelInstGenerator::decode_address_to_instruction(address,
                                                                  instr);
    const uint64_t ra_res =
        pnm::sls::ChannelInstGenerator::encode_address_from_instruction(instr);

    ASSERT_TRUE(address == ra_res);
  }
}

TEST(Instruction, EncodeDecodeInstr) {
  std::default_random_engine generator(std::random_device{}());
  std::uniform_int_distribution<uint64_t> distrib(
      0, std::numeric_limits<uint64_t>::max());

  constexpr uint64_t instr_mask =
      (INSTR_OPCODE_MASK << INSTR_OPCODE_OFFSET) +
      (INSTR_LOCAL_BIT_MASK << INSTR_LOCAL_BIT_OFFSET) +
      (INSTR_OUTPUT_IDX_MASK << INSTR_OUTPUT_IDX_OFFSET) +
      (INSTR_TRACE_END_MASK << INSTR_TRACE_END_OFFSET) +
      (INSTR_ROW_MASK << INSTR_ROW_OFFSET) +
      (INSTR_BANKGROUP_MASK << INSTR_BANKGROUP_OFFSET) +
      (INSTR_BANK_MASK << INSTR_BANK_OFFSET) +
      (INSTR_COL_MASK << INSTR_COL_OFFSET);

  for (int i = 0; i < 30000000; ++i) {
    /* mask out instruction's reserved bits */
    const uint64_t instr_word = distrib(generator) & instr_mask;
    const pnm::sls::RankedInstGenerator::DecodedInst instr =
        pnm::sls::RankedInstGenerator::decode_instruction(instr_word);
    const uint64_t instr_res =
        pnm::sls::RankedInstGenerator::encode_instruction(instr);

    EXPECT_EQ(instr_word, instr_res);
  }
}

const uint64_t ZERO_OFFSET_ON_RANK = 0;
const uint64_t RANDOM_OFFSET = 30;

using pnm::sls::device::topo;

TEST(RankInterleaving, FirstRankInPair) {
  auto *first_rank_address = reinterpret_cast<uint8_t *>(0x100ULL);
  // clang-format off
  /* Cases
    |                          128Kb chunk0                                     |
    |R0 (subchunk0)    R2          R0        R2             R0           R2     |
    |============|-----------|============|-----|.....|=============|-----------|
    ^   ^        ^           ^    ^                                 ^
    |   |        |           |    |                                 |
    |   |        |           |    |                                 |
   C1  C2       C3          C4   C5                                C6

   chunk0 starts from R0.
   1. C1, C2, C3 addresses are not changed by interleaving
   2. C4, C5, C6 addresses are interleaved, as they are placed in non-first R0's
   subchunk
  */
  // clang-format on
  pnm::sls::device::InterleavedPointer gen1(first_rank_address, 0);
  const uint64_t C1 = ZERO_OFFSET_ON_RANK;
  const uint64_t R1 = C1;
  gen1 += C1;
  EXPECT_EQ(gen1.as<uint8_t>(), first_rank_address + R1);

  pnm::sls::device::InterleavedPointer gen2(first_rank_address, 0);
  const uint64_t C2 = ZERO_OFFSET_ON_RANK + RANDOM_OFFSET;
  const uint64_t R2 = C2;
  gen2 += C2;
  EXPECT_EQ(gen2.as<uint8_t>(), first_rank_address + R2);

  pnm::sls::device::InterleavedPointer gen3(first_rank_address, 0);
  const uint64_t C3 = topo().RankInterleavingSize - 1;
  const uint64_t R3 = C3;
  gen3 += C3;
  EXPECT_EQ(gen3.as<uint8_t>(), first_rank_address + R3);

  pnm::sls::device::InterleavedPointer gen4(first_rank_address, 0);
  const uint64_t C4 = topo().RankInterleavingSize;
  const uint64_t R4 = topo().RankInterleavingSize * 2;
  gen4 += C4;
  EXPECT_EQ(gen4.as<uint8_t>(), first_rank_address + R4);

  pnm::sls::device::InterleavedPointer gen5(first_rank_address, 0);
  const uint64_t C5 = topo().RankInterleavingSize + RANDOM_OFFSET;
  const uint64_t R5 = topo().RankInterleavingSize * 2 + RANDOM_OFFSET;
  gen5 += C5;
  EXPECT_EQ(gen5.as<uint8_t>(), first_rank_address + R5);

  static constexpr auto ChannelInterleavingSize = 1ULL
                                                  << (pnm::hw::HA_XOR_BIT + 1);

  pnm::sls::device::InterleavedPointer gen6(first_rank_address, 0);
  const uint64_t C6 = ChannelInterleavingSize / 2 - 1;
  const uint64_t R6 = ChannelInterleavingSize - topo().RankInterleavingSize - 1;
  gen6 += C6;
  EXPECT_EQ(gen6.as<uint8_t>(), first_rank_address + R6);
  // clang-format off
  /* Cases
    |           chunk0            |                          chunk1                           |
    |                 R0      R2  |  R2         R0         R2             R2         R0       |
    |--------------|=======|------|------|===============|-----|.......|------|==============||
                                         ^     ^                              ^       ^      ^
                                         |     |                              |       |      |
                                         |     |                              |       |      |
                                        C7    C8                             C9      C10    C11

    chunk1 starts from R2.
    All addresses are interleaved as they are not in first subchunk of R0 in
    general. They are also in non-first chunk (chunk0 on our case), so we apply
    interchunk leaving to addresses.
  */
  // clang-format on
  pnm::sls::device::InterleavedPointer gen7(first_rank_address, 0);
  const uint64_t C7 = ChannelInterleavingSize / 2;
  const uint64_t R7 = ChannelInterleavingSize + topo().RankInterleavingSize;
  gen7 += C7;
  EXPECT_EQ(gen7.as<uint8_t>(), first_rank_address + R7);

  pnm::sls::device::InterleavedPointer gen8(first_rank_address, 0);
  const uint64_t C8 = ChannelInterleavingSize / 2 + RANDOM_OFFSET;
  const uint64_t R8 =
      ChannelInterleavingSize + topo().RankInterleavingSize + RANDOM_OFFSET;
  gen8 += C8;
  EXPECT_EQ(gen8.as<uint8_t>(), first_rank_address + R8);

  pnm::sls::device::InterleavedPointer gen9(first_rank_address, 0);
  const uint64_t C9 = ChannelInterleavingSize - topo().RankInterleavingSize;
  const uint64_t R9 = 2 * ChannelInterleavingSize - topo().RankInterleavingSize;
  gen9 += C9;
  EXPECT_EQ(gen9.as<uint8_t>(), first_rank_address + R9);

  pnm::sls::device::InterleavedPointer gen10(first_rank_address, 0);
  const uint64_t C10 =
      ChannelInterleavingSize - topo().RankInterleavingSize + RANDOM_OFFSET;
  const uint64_t R10 =
      2 * ChannelInterleavingSize - topo().RankInterleavingSize + RANDOM_OFFSET;
  gen10 += C10;
  EXPECT_EQ(gen10.as<uint8_t>(), first_rank_address + R10);

  pnm::sls::device::InterleavedPointer gen11(first_rank_address, 0);
  const uint64_t C11 = ChannelInterleavingSize - 1;
  const uint64_t R11 = 2 * ChannelInterleavingSize - 1;
  gen11 += C11;
  EXPECT_EQ(gen11.as<uint8_t>(), first_rank_address + R11);
  // clang-format off
  /* Cases
    |      chunk0    |         chunk1           |                         chunk2                        |
    |                |                   R0     |       R0          R2           R0                     |
    |----------------|-------------|===========|||==============|---------|===============|-------------|
                                                 ^       ^                ^       ^
                                                 |       |                |       |
                                                 |       |                |       |
                                                C12     C13              C14     C15
    Addresses are not in first chunk and subchunk.
    Apply inter chunk and subchunk leavings.
  */
  // clang-format on
  pnm::sls::device::InterleavedPointer gen12(first_rank_address, 0);
  const uint64_t C12 = ChannelInterleavingSize;
  const uint64_t R12 = 2 * ChannelInterleavingSize;
  gen12 += C12;
  EXPECT_EQ(gen12.as<uint8_t>(), first_rank_address + R12);

  pnm::sls::device::InterleavedPointer gen13(first_rank_address, 0);
  const uint64_t C13 = ChannelInterleavingSize + RANDOM_OFFSET;
  const uint64_t R13 = 2 * ChannelInterleavingSize + RANDOM_OFFSET;
  gen13 += C13;
  EXPECT_EQ(gen13.as<uint8_t>(), first_rank_address + R13);

  pnm::sls::device::InterleavedPointer gen14(first_rank_address, 0);
  const uint64_t C14 = ChannelInterleavingSize + topo().RankInterleavingSize;
  const uint64_t R14 =
      2 * (ChannelInterleavingSize + topo().RankInterleavingSize);
  gen14 += C14;
  EXPECT_EQ(gen14.as<uint8_t>(), first_rank_address + R14);

  pnm::sls::device::InterleavedPointer gen15(first_rank_address, 0);
  const uint64_t C15 =
      ChannelInterleavingSize + topo().RankInterleavingSize + RANDOM_OFFSET;
  const uint64_t R15 =
      2 * (ChannelInterleavingSize + topo().RankInterleavingSize) +
      RANDOM_OFFSET;
  gen15 += C15;
  EXPECT_EQ(gen15.as<uint8_t>(), first_rank_address + R15);
}

TEST(RankInterleaving, SecondRankInPair) {
  auto *second_rank_address = reinterpret_cast<uint8_t *>(0x180ULL);
  // clang-format off
  /* Cases
    |                                 chunk0                                        |
    |   R0    R2 (subchunk0)  R0          R2                  R0           R2       |
    |--------|=============|-------|=============|.......|----------|==============||
             ^      ^      ^       ^      ^                              ^         ^
             |      |      |       |      |                              |         |
             |      |      |       |      |                              |         |
            C1     C2     C3      C4     C5                             C6        C7

    1. C1, C2, C3 placed in first RANK's subchunk and first chunk (chunk0), they're not interleaved.
    2. C4, C5, C6, C7 placed in non-first subchunk of first chunk (chunk0), they're interleaved by subchunk.
  */
  // clang-format on
  auto *init_second_rank_address =
      second_rank_address - topo().RankInterleavingSize;
  pnm::sls::device::InterleavedPointer gen1(init_second_rank_address, 1);
  const uint64_t C1 = ZERO_OFFSET_ON_RANK;
  const uint64_t R1 = C1;
  gen1 += C1;
  EXPECT_EQ(gen1.as<uint8_t>(), second_rank_address + R1);

  pnm::sls::device::InterleavedPointer gen2(init_second_rank_address, 1);
  const uint64_t C2 = ZERO_OFFSET_ON_RANK + RANDOM_OFFSET;
  const uint64_t R2 = C2;
  gen2 += C2;
  EXPECT_EQ(gen2.as<uint8_t>(), second_rank_address + R2);

  pnm::sls::device::InterleavedPointer gen3(init_second_rank_address, 1);
  const uint64_t C3 = topo().RankInterleavingSize - 1;
  const uint64_t R3 = C3;
  gen3 += C3;
  EXPECT_EQ(gen3.as<uint8_t>(), second_rank_address + R3);

  pnm::sls::device::InterleavedPointer gen4(init_second_rank_address, 1);
  const uint64_t C4 = topo().RankInterleavingSize;
  const uint64_t R4 = 2 * topo().RankInterleavingSize;
  gen4 += C4;
  EXPECT_EQ(gen4.as<uint8_t>(), second_rank_address + R4);

  pnm::sls::device::InterleavedPointer gen5(init_second_rank_address, 1);
  const uint64_t C5 = topo().RankInterleavingSize + RANDOM_OFFSET;
  const uint64_t R5 = 2 * topo().RankInterleavingSize + RANDOM_OFFSET;
  gen5 += C5;
  EXPECT_EQ(gen5.as<uint8_t>(), second_rank_address + R5);

  static constexpr auto ChannelInterleavingSize = 1ULL
                                                  << (pnm::hw::HA_XOR_BIT + 1);

  pnm::sls::device::InterleavedPointer gen6(init_second_rank_address, 1);
  const uint64_t C6 =
      ChannelInterleavingSize / 2 - topo().RankInterleavingSize + RANDOM_OFFSET;
  const uint64_t R6 =
      ChannelInterleavingSize - 2 * topo().RankInterleavingSize + RANDOM_OFFSET;
  gen6 += C6;
  EXPECT_EQ(gen6.as<uint8_t>(), second_rank_address + R6);

  pnm::sls::device::InterleavedPointer gen7(init_second_rank_address, 1);
  const uint64_t C7 = ChannelInterleavingSize / 2 - 1;
  const uint64_t R7 = ChannelInterleavingSize - topo().RankInterleavingSize - 1;
  gen7 += C7;
  EXPECT_EQ(gen7.as<uint8_t>(), second_rank_address + R7);
  // clang-format off
  /* Cases
    |      chunk0       |                                  chunk1                                 |
    |               R2  |      R2          R0          R2                      R2           R0    |
    |-----------|=======||============|----------|=============|........|==============|----------|
                         ^            ^          ^                           ^
                         |            |          |                           |
                         |            |          |                           |
                        C8           C9         C10                         C11
    All addresses are places in non-first subchunk and non-first chunk.
    So, they're interleaved by chunk and subchunk.
  */
  // clang-format on
  pnm::sls::device::InterleavedPointer gen8(init_second_rank_address, 1);
  const uint64_t C8 = ChannelInterleavingSize / 2;
  const uint64_t R8 = ChannelInterleavingSize - topo().RankInterleavingSize;
  gen8 += C8;
  EXPECT_EQ(gen8.as<uint8_t>(), second_rank_address + R8);

  pnm::sls::device::InterleavedPointer gen9(init_second_rank_address, 1);
  const uint64_t C9 =
      ChannelInterleavingSize / 2 + topo().RankInterleavingSize - 1;
  const uint64_t R9 = ChannelInterleavingSize - 1;
  gen9 += C9;
  EXPECT_EQ(gen9.as<uint8_t>(), second_rank_address + R9);

  pnm::sls::device::InterleavedPointer gen10(init_second_rank_address, 1);
  const uint64_t C10 =
      ChannelInterleavingSize / 2 + topo().RankInterleavingSize;
  const uint64_t R10 = ChannelInterleavingSize + topo().RankInterleavingSize;
  gen10 += C10;
  EXPECT_EQ(gen10.as<uint8_t>(), second_rank_address + R10);

  pnm::sls::device::InterleavedPointer gen11(init_second_rank_address, 1);
  const uint64_t C11 =
      ChannelInterleavingSize - topo().RankInterleavingSize + RANDOM_OFFSET;
  const uint64_t R11 = 2 * ChannelInterleavingSize -
                       3 * topo().RankInterleavingSize + RANDOM_OFFSET;
  gen11 += C11;
  EXPECT_EQ(gen11.as<uint8_t>(), second_rank_address + R11);
  // clang-format off
  /* Cases
    |   chunk0  |      chunk1      |                  chunk2
    |           |              R0  |   R0         R2
    |-----------|----------|-------|-------|===============|.....................
                                           ^     ^
                                           |     |
                                           |     |
                                          C12   C13
    All addresses are places in non-first subchunk and non-first chunk.
    So, they're interleaved by chunk and subchunk.
  */
  // clang-format on
  pnm::sls::device::InterleavedPointer gen12(init_second_rank_address, 1);
  const uint64_t C12 = ChannelInterleavingSize;
  const uint64_t R12 = 2 * ChannelInterleavingSize;
  gen12 += C12;
  EXPECT_EQ(gen12.as<uint8_t>(), second_rank_address + R12);

  pnm::sls::device::InterleavedPointer gen13(init_second_rank_address, 1);
  const uint64_t C13 = ChannelInterleavingSize + RANDOM_OFFSET;
  const uint64_t R13 = 2 * ChannelInterleavingSize + RANDOM_OFFSET;
  gen13 += C13;
  EXPECT_EQ(gen13.as<uint8_t>(), second_rank_address + R13);
}
