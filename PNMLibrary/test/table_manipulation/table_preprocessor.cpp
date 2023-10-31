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

#include "fixtures.h"
#include "ptr_rw.h"

#include "secure/common/sls_io.h"
#include "secure/encryption/sls_dlrm_preprocessor.h"

#include "common/topology_constants.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

using pnm::sls::device::topo;

/*! \brief Text fixture that create and store embedding tables. */

TEST_F(TablePreprocessing, MemcheckLoadWithoutTag) {
  pnm::sls::secure::DLRMPreprocessor<uint32_t, std::ifstream,
                                     pnm::sls::secure::TrivialMemoryWriter>
      table(root + "embedding.bin", info.rows(), info.cols());

  auto rbytes = rank_bytes();

  auto *ptr = static_cast<uint8_t *>(
      aligned_alloc(topo().RankInterleavingSize, rbytes));
  std::fill(ptr, ptr + rbytes, 255);

  table.load(ptr, false);

  free(ptr);
}

TEST_F(TablePreprocessing, MemcheckLoadWithTag) {
  pnm::sls::secure::DLRMPreprocessor<uint32_t, std::ifstream,
                                     pnm::sls::secure::TrivialMemoryWriter>
      table(root + "embedding.bin", info.rows(), info.cols());

  auto rank_bytes = rank_bytes_with_tag();

  auto *ptr = static_cast<uint8_t *>(
      aligned_alloc(topo().RankInterleavingSize, rank_bytes));
  std::fill(ptr, ptr + rank_bytes, 255);

  table.load(ptr, true);

  free(ptr);
}

TEST_F(TablePreprocessing, OTPEvalCheck) {
  std::vector<char> mem(data_count() * sizeof(uint32_t), 0);

  pnm::sls::secure::DLRMPreprocessor<uint32_t, PtrReader, PtrWriter> table(
      mem.data(), info.rows(), info.cols());
  table.load(mem.data(), false); // Now in mem we have -OTP

  auto *it = reinterpret_cast<uint32_t *>(mem.data());
  while (it != reinterpret_cast<uint32_t *>(mem.data() + mem.size())) {
    // Here we assume that probability of OTP == 0 is extremely close to zero
    ASSERT_FALSE(
        std::all_of(it, it + info.cols(), [](auto v) { return v == 0; }));
    std::advance(it, info.cols());
  }
}

TEST(TablePreprocessor, BufferSize) {
  const pnm::sls::secure::DLRMPreprocessor<uint64_t, PtrReader, PtrWriter> t64(
      nullptr, {5, 10}, 8);
  EXPECT_EQ(t64.encrypted_buffer_size(false), 8 * (5 + 10) * sizeof(uint64_t));
  EXPECT_EQ(t64.encrypted_buffer_size(true),
            (8 + 2) * (5 + 10) * sizeof(uint64_t));

  const pnm::sls::secure::DLRMPreprocessor<uint32_t, PtrReader, PtrWriter> t32(
      nullptr, {2, 4, 16}, 16);
  EXPECT_EQ(t32.encrypted_buffer_size(false),
            16 * (2 + 4 + 16) * sizeof(uint32_t));
  EXPECT_EQ(t32.encrypted_buffer_size(true),
            (16 + 4) * (2 + 4 + 16) * sizeof(uint32_t));
}
