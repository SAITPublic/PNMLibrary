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

#include "common/file_descriptor_holder.h"
#include "common/temporary_filesystem_object.h"

#include "pnmlib/common/error.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <fcntl.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

TEST(FileDescriptorHolder, CheckValidity) {
  const std::string fake_file_name = "this/file/does/not/exist";

  ASSERT_THROW(
      const pnm::utils::FileDescriptorHolder fd(fake_file_name, O_RDONLY),
      pnm::error::IO);

  const pnm::utils::FileDescriptorHolder empty_fd;
  ASSERT_FALSE(empty_fd.is_valid());
  ASSERT_FALSE(empty_fd);

  const pnm::utils::TemporaryFile temp_file{};
  ASSERT_TRUE(temp_file);
  fmt::print("The temporary file: {}\n", temp_file.name());

  pnm::utils::FileDescriptorHolder fd(temp_file.name(), O_RDONLY);
  ASSERT_TRUE(fd.is_valid());
  ASSERT_TRUE(fd);

  const pnm::utils::FileDescriptorHolder moved_fd(std::move(fd));
  ASSERT_TRUE(moved_fd);
  ASSERT_FALSE(fd);
}

TEST(FileDescriptorHolder, CheckGet) {
  const pnm::utils::FileDescriptorHolder empty_fd;
  ASSERT_THROW(empty_fd.get(), pnm::error::IO);
  ASSERT_THROW(*empty_fd, pnm::error::IO);

  const pnm::utils::TemporaryFile temp_file{};
  ASSERT_TRUE(temp_file);
  fmt::print("The temporary file: {}\n", temp_file.name());

  pnm::utils::FileDescriptorHolder fd(temp_file.name(), O_RDONLY);
  const int raw_fd = fd.get();
  ASSERT_EQ(raw_fd, *fd);

  const pnm::utils::FileDescriptorHolder moved_fd(std::move(fd));
  ASSERT_EQ(raw_fd, *moved_fd);

  ASSERT_THROW(*fd, pnm::error::IO);
}

TEST(FileDescriptorHolder, CheckIO) {
  const pnm::utils::TemporaryFile temp_file{};
  ASSERT_TRUE(temp_file);
  fmt::print("The temporary file: {}\n", temp_file.name());

  const char *test_string = "Hello, world!";
  const size_t test_string_size = 13;

  const pnm::utils::FileDescriptorHolder write_fd(temp_file.name(), O_WRONLY);
  ASSERT_EQ(write(*write_fd, test_string, test_string_size), test_string_size);

  const pnm::utils::FileDescriptorHolder read_fd(temp_file.name(), O_RDONLY);
  char buf[test_string_size + 1];
  ASSERT_EQ(read(*read_fd, buf, test_string_size), test_string_size);
  ASSERT_EQ(strncmp(test_string, buf, test_string_size), 0);
}
