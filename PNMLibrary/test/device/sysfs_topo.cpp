/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 *  This software is proprietary of Samsung Electronics. No part of this
 * software, either material or conceptual may be copied or distributed,
 * transmitted, transcribed, stored in a retrieval system or translated into any
 * human or computer language in any form by any means, electronic, mechanical,
 * manual or otherwise, or disclosed to third parties without the express
 * written permission of Samsung Electronics.
 *
 */

#include "common/misc_control.h"
#include "common/topology_constants.h"

#include <gtest/gtest.h>

#include <linux/sls_resources.h>

#include <filesystem>
#include <utility>

#define ABS_PATH(rel_path) SLS_SYSFS_ROOT "/" rel_path

//[TODO: @y-lavrinenko] Add fix to reinitialize module with predefined topology

TEST(TopologySysfs, PathExist) {
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_NUM_OF_RANKS_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_NUM_OF_CS_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_NUM_OF_CHANNEL_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_INSTRUCTION_SIZE_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_DATA_SIZE_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_ALIGNED_TAG_SIZE_PATH)));
  ASSERT_TRUE(
      std::filesystem::exists(ABS_PATH(ATTR_RANK_INTERLEAVING_SIZE_PATH)));
  ASSERT_TRUE(
      std::filesystem::exists(ABS_PATH(ATTR_CHANNEL_INTERLEAVING_SIZE_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_INTERLEAVING_STRIDE_PATH)));
  ASSERT_TRUE(
      std::filesystem::exists(ABS_PATH(ATTR_POLLING_REGISTER_OFFSET_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_SLS_EXEC_VALUE_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_REG_SLS_EN_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_PSUM_BUFFER_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_TAGS_BUFFER_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_INST_BUFFER_PATH)));
}

using pnm::device::topo;

template <typename T>
void check_value(const std::filesystem::path &path, T golden) {
  T value{};
  EXPECT_NO_THROW(pnm::control::utils::read_file(path, value))
      << "Exception threw at read " << path;
  EXPECT_EQ(value, golden) << "Value mismatch for " << path;
}

TEST(TopologySysfs, MemoryTopology) {
  check_value(ABS_PATH(ATTR_NUM_OF_RANKS_PATH), topo().NumOfRanks);
  check_value(ABS_PATH(ATTR_NUM_OF_CS_PATH), topo().NumOfCS);
  check_value(ABS_PATH(ATTR_NUM_OF_CHANNEL_PATH), topo().NumOfChannels);
}

TEST(TopologySysfs, InstructionSizes) {
  check_value(ABS_PATH(ATTR_INSTRUCTION_SIZE_PATH), topo().InstructionSize);
  check_value(ABS_PATH(ATTR_DATA_SIZE_PATH), topo().DataSize);
  check_value(ABS_PATH(ATTR_ALIGNED_TAG_SIZE_PATH), topo().AlignedTagSize);
}

TEST(TopologySysfs, Interleaving) {
  check_value(ABS_PATH(ATTR_RANK_INTERLEAVING_SIZE_PATH),
              topo().RankInterleavingSize);
  check_value(ABS_PATH(ATTR_CHANNEL_INTERLEAVING_SIZE_PATH),
              topo().ChannelInterleavingSize);
  check_value(ABS_PATH(ATTR_INTERLEAVING_STRIDE_PATH),
              topo().InterleavingStrideLength);
}

TEST(TopologySysfs, Registers) {
  check_value(ABS_PATH(ATTR_POLLING_REGISTER_OFFSET_PATH), topo().RegPolling);
  check_value(ABS_PATH(ATTR_SLS_EXEC_VALUE_PATH), topo().RegSLSExec);
  check_value(ABS_PATH(ATTR_REG_SLS_EN_PATH), topo().RegSLSEn);
}

template <typename T>
void check_value(const std::filesystem::path &path, std::pair<T, T> golden) {
  std::pair<T, T> value{};
  EXPECT_NO_THROW(
      pnm::control::utils::read_file(path, value.first, value.second))
      << "Exception threw at read " << path;
  EXPECT_EQ(value, golden) << "Value mismatch for " << path;
}

TEST(TopologySysfs, Buffers) {
  check_value(ABS_PATH(ATTR_PSUM_BUFFER_PATH),
              std::pair{topo().NumOfPSumBuf, topo().PSumBufSize});
  check_value(ABS_PATH(ATTR_TAGS_BUFFER_PATH),
              std::pair{topo().NumOfTagsBuf, topo().TagsBufSize});
  check_value(ABS_PATH(ATTR_INST_BUFFER_PATH),
              std::pair{topo().NumOfInstBuf, topo().InstBufSize});
}
