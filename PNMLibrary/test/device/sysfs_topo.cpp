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

#define ABS_PATH(rel_path) SLS_SYSFS_ROOT "/" rel_path

//[TODO: @y-lavrinenko] Add fix to reinitialize module with predefined topology

TEST(TopologySysfs, PathExist) {
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_NUM_OF_CUNITS_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_INSTRUCTION_SIZE_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_DATA_SIZE_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_ALIGNED_TAG_SIZE_PATH)));
  ASSERT_TRUE(
      std::filesystem::exists(ABS_PATH(ATTR_POLLING_REGISTER_OFFSET_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_SLS_EXEC_VALUE_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_REG_SLS_EN_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_PSUM_BUFFER_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_TAGS_BUFFER_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_INST_BUFFER_PATH)));
  ASSERT_TRUE(std::filesystem::exists(ABS_PATH(ATTR_BUFFER_SIZE_PATH)));
}

using pnm::sls::device::topo;

template <typename T>
void check_value(const std::filesystem::path &path, T golden) {
  T value{};
  EXPECT_NO_THROW(pnm::control::read_file(path, value))
      << "Exception threw at read " << path;
  EXPECT_EQ(value, golden) << "Value mismatch for " << path;
}

TEST(TopologySysfs, MemoryTopology) {
  check_value(ABS_PATH(ATTR_NUM_OF_CUNITS_PATH), topo().NumOfCUnits);
}

TEST(TopologySysfs, InstructionSizes) {
  check_value(ABS_PATH(ATTR_INSTRUCTION_SIZE_PATH), topo().InstructionSize);
  check_value(ABS_PATH(ATTR_DATA_SIZE_PATH), topo().DataSize);
  check_value(ABS_PATH(ATTR_ALIGNED_TAG_SIZE_PATH), topo().AlignedTagSize);
}

TEST(TopologySysfs, Registers) {
  check_value(ABS_PATH(ATTR_POLLING_REGISTER_OFFSET_PATH), topo().RegPolling);
  check_value(ABS_PATH(ATTR_SLS_EXEC_VALUE_PATH), topo().RegSlsExec);
  check_value(ABS_PATH(ATTR_REG_SLS_EN_PATH), topo().RegSlsEn);
}

TEST(TopologySysfs, Buffers) {
  check_value(ABS_PATH(ATTR_PSUM_BUFFER_PATH), topo().NumOfPSumBuf);
  check_value(ABS_PATH(ATTR_TAGS_BUFFER_PATH), topo().NumOfTagsBuf);
  check_value(ABS_PATH(ATTR_INST_BUFFER_PATH), topo().NumOfInstBuf);
  check_value(ABS_PATH(ATTR_BUFFER_SIZE_PATH), topo().PSumBufSize);
  check_value(ABS_PATH(ATTR_BUFFER_SIZE_PATH), topo().TagsBufSize);
  check_value(ABS_PATH(ATTR_BUFFER_SIZE_PATH), topo().InstBufSize);
}
