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

#include "secure/plain/device/trivial_cpu.h"

#include "common/cpu_sls.h"

#include "pnmlib/secure/base_device.h"

#include "pnmlib/common/misc_utils.h"
#include "pnmlib/common/rowwise_view.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

struct UntrustedDev : public ::testing::Test,
                      public sls::secure::TrivialCPU<uint32_t> {
  const std::vector<uint32_t> rows{5, 10, 5};
};
using sls::secure::DeviceArguments;
using sls::secure::TrivialCPUArgs;

TEST(DeviceArguments, CreateRead) {
  const std::vector<uint32_t> rows{4, 5, 6};
  static constexpr auto sparse_feature_size = 18;
  DeviceArguments args(
      TrivialCPUArgs{pnm::make_view(rows), sparse_feature_size, false});

  ASSERT_EQ(args.get<const TrivialCPUArgs>().sparse_feature_size,
            sparse_feature_size);

  ASSERT_EQ(args.get<const TrivialCPUArgs>().rows.size(), rows.size());
}

TEST_F(UntrustedDev, CreateLoad) {
  static constexpr auto sparse_feature_size = 8;
  const DeviceArguments args(TrivialCPUArgs{{}, sparse_feature_size, false});
  init(&args);

  std::vector<uint32_t> data(10, 0);

  load(reinterpret_cast<const char *>(data.data()),
       pnm::utils::byte_size_of_container(data));
}

TEST_F(UntrustedDev, SLSNoTag) {
  std::vector data{1,   2,   3,   4,   -1,  -2,  -3,  -4,  5,   6,   7,   8,
                   -5,  -6,  -7,  -8,  6,   8,   11,  13,  10,  20,  30,  40,
                   -10, -20, -30, -40, 50,  60,  70,  80,  -50, -60, -70, -80,
                   60,  80,  110, 130, 10,  20,  30,  40,  -10, -20, -30, -40,
                   50,  60,  70,  80,  -50, -60, -70, -80, 60,  80,  110, 130,
                   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   1,   0,
                   1,   0,   0,   0,   1,   1,   1,   1};

  const DeviceArguments args(TrivialCPUArgs{pnm::make_view(rows), 4, false});
  init(&args);

  load(reinterpret_cast<const char *>(data.data()),
       pnm::utils::byte_size_of_container(data));

  std::vector<uint32_t> indices{1, 3, 4, 0, 1, 4, 0, 1, 2, 2, 3, 4, 4, 5,
                                6, 1, 4, 9, 0, 1, 3, 1, 2, 3, 0, 1, 4};
  static constexpr auto minibatch_size = 3;
  static constexpr auto num_tables = 3;
  static constexpr auto sparse_feature_size = 4;
  static constexpr auto num_lookup = 3;

  std::vector<uint32_t> lengths(indices.size() / num_lookup, num_lookup);

  std::vector<uint32_t> psum(minibatch_size * num_tables * sparse_feature_size,
                             0);

  run_sls(minibatch_size, pnm::make_const_view(lengths),
          pnm::make_const_view(indices), pnm::make_view(psum));

  const std::vector<uint32_t> golden{0,   0,   1,   1,   6,   8,   11,  13, 5,
                                     6,   7,   8,   60,  80,  110, 130, 60, 80,
                                     110, 130, 110, 140, 190, 220, 1,   1,  0,
                                     0,   1,   1,   1,   0,   1,   2,   1,  1};
  ASSERT_EQ(psum, golden);
}

TEST_F(UntrustedDev, SLSWithTag) {
  std::vector data{
      1,   2,   3,   4,   0, 0, 0, 1,   -1,  -2,  -3,  -4,  0, 0, 0, 2,
      5,   6,   7,   8,   0, 0, 0, 3,   -5,  -6,  -7,  -8,  0, 0, 0, 4,
      6,   8,   11,  13,  0, 0, 0, 5,   10,  20,  30,  40,  0, 0, 0, 10,
      -10, -20, -30, -40, 0, 0, 0, 11,  50,  60,  70,  80,  0, 0, 0, 12,
      -50, -60, -70, -80, 0, 0, 0, 13,  60,  80,  110, 130, 0, 0, 0, 14,
      10,  20,  30,  40,  0, 0, 0, 15,  -10, -20, -30, -40, 0, 0, 0, 16,
      50,  60,  70,  80,  0, 0, 0, 17,  -50, -60, -70, -80, 0, 0, 0, 18,
      60,  80,  110, 130, 0, 0, 0, 19,  0,   0,   0,   0,   0, 0, 0, 100,
      0,   1,   0,   0,   0, 0, 0, 101, 0,   0,   1,   0,   0, 0, 0, 102,
      1,   0,   0,   0,   0, 0, 0, 103, 1,   1,   1,   1,   0, 0, 0, 104};

  const DeviceArguments args(TrivialCPUArgs{pnm::make_view(rows), 4, true});
  init(&args);

  load(reinterpret_cast<const char *>(data.data()),
       pnm::utils::byte_size_of_container(data));

  std::vector<uint32_t> indices{1, 3, 4, 0, 1, 4, 0, 1, 2, 2, 3, 4, 4, 5,
                                6, 1, 4, 9, 0, 1, 3, 1, 2, 3, 0, 1, 4};

  static constexpr auto minibatch_size = 3;
  static constexpr auto num_tables = 3;
  static constexpr auto sparse_feature_size = 8;
  static constexpr auto num_lookup = 3;

  std::vector<uint32_t> lengths(indices.size() / num_lookup, num_lookup);

  std::vector<uint32_t> psum(minibatch_size * num_tables * sparse_feature_size,
                             0);

  run_sls_with_tag(minibatch_size, pnm::make_const_view(lengths),
                   pnm::make_const_view(indices), pnm::make_view(psum));

  const std::vector<uint32_t> golden{
      0, 0, 1, 1, 6, 8, 11, 13, 5, 6, 7, 8, 60, 80, 110, 130, 60, 80, 110, 130,
      110, 140, 190, 220, 1, 1, 0, 0, 1, 1, 1, 0, 1, 2, 1, 1,
      // Tags section
      0, 0, 0, 11, 0, 0, 0, 8, 0, 0, 0, 6, 0, 0, 0, 39, 0, 0, 0, 45, 0, 0, 0,
      44, 0, 0, 0, 304, 0, 0, 0, 306, 0, 0, 0, 305};
  ASSERT_EQ(psum, golden);
}

TEST(UntrustedDevApiCall, SLSNoTag) {
  std::vector data{1,   2,   3,   4,   -1,  -2,  -3,  -4,  5,   6,   7,   8,
                   -5,  -6,  -7,  -8,  6,   8,   11,  13,  10,  20,  30,  40,
                   -10, -20, -30, -40, 50,  60,  70,  80,  -50, -60, -70, -80,
                   60,  80,  110, 130, 10,  20,  30,  40,  -10, -20, -30, -40,
                   50,  60,  70,  80,  -50, -60, -70, -80, 60,  80,  110, 130,
                   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   1,   0,
                   1,   0,   0,   0,   1,   1,   1,   1};
  sls::secure::TrivialCPU<uint32_t> dev;
  const std::vector<uint32_t> rows_long{5, 10, 5};
  const DeviceArguments args(
      TrivialCPUArgs{pnm::make_view(rows_long), 4, false});
  dev.init(&args);

  dev.load(reinterpret_cast<const char *>(data.data()),
           pnm::utils::byte_size_of_container(data));

  static constexpr auto minibatch_size = 3;
  static constexpr auto num_tables = 3;
  static constexpr auto sparse_feature_size = 4;
  static constexpr auto num_lookup = 3U;

  { // Equal batch size for each tables
    std::vector<uint32_t> indices{1, 3, 4, 0, 1, 4, 0, 1, 2, 2, 3, 4, 4, 5,
                                  6, 1, 4, 9, 0, 1, 3, 1, 2, 3, 0, 1, 4};
    std::vector<uint32_t> lengths(indices.size() / num_lookup, num_lookup);
    std::vector<uint32_t> psum(
        minibatch_size * num_tables * sparse_feature_size, 0);
    const std::vector<uint32_t> rows{5, 10, 5};

    dev.run(minibatch_size, pnm::make_const_view(lengths),
            pnm::make_const_view(indices), pnm::make_view(psum));

    const std::vector<uint32_t> golden{
        0,  0,  1,   1,   6,  8,  11,  13,  5,   6,   7,   8,
        60, 80, 110, 130, 60, 80, 110, 130, 110, 140, 190, 220,
        1,  1,  0,   0,   1,  1,  1,   0,   1,   2,   1,   1};
    ASSERT_EQ(psum, golden);
  }
  { // Various batch size for each tables
    std::vector<uint32_t> indices{0, 1, 3, 4, 0, 0, 1, 4, 0, 0, 1, 2, 2, 3,
                                  4, 4, 5, 6, 1, 4, 9, 1, 3, 1, 3, 1, 4};
    std::vector<uint32_t> lengths{4, 4, 4, 3, 3, 3, 2, 2, 2};
    std::vector<uint32_t> psum(
        minibatch_size * num_tables * sparse_feature_size, 0);
    const std::vector<uint32_t> rows{5, 10, 5};

    dev.run(minibatch_size, pnm::make_const_view(lengths),
            pnm::make_const_view(indices), pnm::make_view(psum));

    const std::vector<uint32_t> golden{
        1,  2,  4,   5,   7,  10, 14,  17,  6,   8,   10,  12,
        60, 80, 110, 130, 60, 80, 110, 130, 110, 140, 190, 220,
        1,  1,  0,   0,   1,  1,  0,   0,   1,   2,   1,   1};
    ASSERT_EQ(psum, golden);
  }
}

TEST(UntrustedDevApiCall, SLSWithTag) {
  std::vector data{
      1,   2,   3,   4,   0, 0, 0, 1,   -1,  -2,  -3,  -4,  0, 0, 0, 2,
      5,   6,   7,   8,   0, 0, 0, 3,   -5,  -6,  -7,  -8,  0, 0, 0, 4,
      6,   8,   11,  13,  0, 0, 0, 5,   10,  20,  30,  40,  0, 0, 0, 10,
      -10, -20, -30, -40, 0, 0, 0, 11,  50,  60,  70,  80,  0, 0, 0, 12,
      -50, -60, -70, -80, 0, 0, 0, 13,  60,  80,  110, 130, 0, 0, 0, 14,
      10,  20,  30,  40,  0, 0, 0, 15,  -10, -20, -30, -40, 0, 0, 0, 16,
      50,  60,  70,  80,  0, 0, 0, 17,  -50, -60, -70, -80, 0, 0, 0, 18,
      60,  80,  110, 130, 0, 0, 0, 19,  0,   0,   0,   0,   0, 0, 0, 100,
      0,   1,   0,   0,   0, 0, 0, 101, 0,   0,   1,   0,   0, 0, 0, 102,
      1,   0,   0,   0,   0, 0, 0, 103, 1,   1,   1,   1,   0, 0, 0, 104};

  sls::secure::TrivialCPU<uint32_t> dev;
  const std::vector<uint32_t> rows_long{5, 10, 5};
  const DeviceArguments args(
      TrivialCPUArgs{pnm::make_view(rows_long), 4, true});
  dev.init(&args);

  dev.load(reinterpret_cast<const char *>(data.data()),
           pnm::utils::byte_size_of_container(data));

  static constexpr auto minibatch_size = 3;
  static constexpr auto num_tables = 3;
  static constexpr auto sparse_feature_size = 8;

  { // Equal batch size for each tables
    std::vector<uint32_t> indices{1, 3, 4, 0, 1, 4, 0, 1, 2, 2, 3, 4, 4, 5,
                                  6, 1, 4, 9, 0, 1, 3, 1, 2, 3, 0, 1, 4};
    std::vector<uint32_t> lengths(indices.size() / 3, 3);
    std::vector<uint32_t> psum(
        minibatch_size * num_tables * sparse_feature_size, 0);
    const std::vector<uint32_t> rows{5, 10, 5};

    dev.run(minibatch_size, pnm::make_const_view(lengths),
            pnm::make_const_view(indices), pnm::make_view(psum));

    const std::vector<uint32_t> golden{
        0, 0, 1, 1, 6, 8, 11, 13, 5, 6, 7, 8, 60, 80, 110, 130, 60, 80, 110,
        130, 110, 140, 190, 220, 1, 1, 0, 0, 1, 1, 1, 0, 1, 2, 1, 1,
        // Tags section
        0, 0, 0, 11, 0, 0, 0, 8, 0, 0, 0, 6, 0, 0, 0, 39, 0, 0, 0, 45, 0, 0, 0,
        44, 0, 0, 0, 304, 0, 0, 0, 306, 0, 0, 0, 305};
    ASSERT_EQ(psum, golden);
  }
  {
    std::vector<uint32_t> indices{0, 1, 3, 4, 0, 0, 1, 4, 0, 0, 1, 2, 2, 3,
                                  4, 4, 5, 6, 1, 4, 9, 1, 3, 1, 3, 1, 4};
    std::vector<uint32_t> lengths{4, 4, 4, 3, 3, 3, 2, 2, 2};
    std::vector<uint32_t> psum(
        minibatch_size * num_tables * sparse_feature_size, 0);
    const std::vector<uint32_t> rows{5, 10, 5};

    dev.run(minibatch_size, pnm::make_const_view(lengths),
            pnm::make_const_view(indices), pnm::make_view(psum));

    const std::vector<uint32_t> golden{
        1, 2, 4, 5, 7, 10, 14, 17, 6, 8, 10, 12, 60, 80, 110, 130, 60, 80, 110,
        130, 110, 140, 190, 220, 1, 1, 0, 0, 1, 1, 0, 0, 1, 2, 1, 1,
        // Tags section
        0, 0, 0, 12, 0, 0, 0, 9, 0, 0, 0, 7, 0, 0, 0, 39, 0, 0, 0, 45, 0, 0, 0,
        44, 0, 0, 0, 204, 0, 0, 0, 204, 0, 0, 0, 205};

    ASSERT_EQ(psum, golden);
  }
}

TEST(SLS, CommonCheck) {
  const std::vector data{1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 4, 3, 2, 1};
  const std::vector indices{0U, 2U, 3U};
  const std::vector golden{1 + 1 + 4, 2 + 2 + 3, 3 + 3 + 2, 4 + 4 + 1};
  std::vector<int> psum(golden.size(), 0);

  SLS<int>::common(
      pnm::make_rowwise_view(data.data(), data.data() + data.size(), 4),
      pnm::make_view(indices), pnm::make_view(psum));
  ASSERT_EQ(psum, golden);
}

TEST(SLS, TagCheck) {
  const std::vector data{1, 2, 3, 4, 0, 0, 0, 1, 1, 2, 3, 4, 0, 0, 0, 2,
                         1, 2, 3, 4, 0, 0, 0, 3, 4, 3, 2, 1, 0, 0, 0, 4};
  const std::vector indices{0U, 2U, 3U};
  const std::vector golden{1 + 1 + 4, 2 + 2 + 3, 3 + 3 + 2, 4 + 4 + 1,
                           0,         0,         0,         8};
  std::vector<int> psum(golden.size(), 0);

  SLS<int>::common(
      pnm::make_rowwise_view(data.data(), data.data() + data.size(), 8, 0, 4),
      pnm::make_view(indices), pnm::make_view(psum.data(), psum.data() + 4));

  SLS<int>::on_tag(
      pnm::make_rowwise_view(data.data(), data.data() + data.size(), 8, 4, 0),
      pnm::make_view(indices),
      pnm::make_view(psum.data() + 4, psum.data() + 8));
  EXPECT_EQ(psum, golden);
}
