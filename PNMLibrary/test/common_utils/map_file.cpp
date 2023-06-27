/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#include "common/mapped_file.h"

#include "pnmlib/common/misc_utils.h"

#include <gtest/gtest.h>

#include <stdlib.h> // NOLINT(modernize-deprecated-headers)

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using pnm::utils::MappedFile;
namespace {

template <typename T, typename Distribution>
std::vector<T> generate_random_vector_impl(size_t vec_size,
                                           Distribution &distrib) {
  std::mt19937_64 engine{std::random_device{}()};
  const auto gen = [&distrib, &engine]() { return distrib(engine); };
  std::vector<T> generated_vec(vec_size);
  std::generate(generated_vec.begin(), generated_vec.end(), gen);
  return generated_vec;
}

template <typename NumberType>
std::vector<NumberType> generate_random_vector(size_t vec_size) {
  static_assert(std::is_arithmetic_v<NumberType>);

  if constexpr (std::is_integral_v<NumberType>) {
    std::uniform_int_distribution<NumberType> distrib(
        std::numeric_limits<NumberType>::lowest(),
        std::numeric_limits<NumberType>::max());
    return generate_random_vector_impl<NumberType, decltype(distrib)>(vec_size,
                                                                      distrib);
  }

  if constexpr (std::is_floating_point_v<NumberType>) {
    std::uniform_real_distribution<NumberType> distrib(
        0.5 * std::numeric_limits<NumberType>::lowest(),
        0.5 * std::numeric_limits<NumberType>::max());
    return generate_random_vector_impl<NumberType, decltype(distrib)>(vec_size,
                                                                      distrib);
  }

  throw std::runtime_error("Generating vector for this type not supported");
}

template <typename T>
void write_to_file(const std::vector<T> &data,
                   const std::filesystem::path &file) {
  std::ofstream out(file, std::ios::out | std::ios::binary);
  out.exceptions(std::ofstream::badbit);
  ASSERT_TRUE(out);
  out.write(reinterpret_cast<const char *>(data.data()),
            pnm::utils::byte_size_of_container(data));
}

template <typename DataType>
void test_mmapped_file(const std::filesystem::path &file,
                       const std::vector<DataType> &data) {
  write_to_file<DataType>(data, file);
  MappedFile init_mmapped_data(file);
  ASSERT_TRUE(init_mmapped_data);
  const MappedFile mmapped_data(std::move(init_mmapped_data));
  ASSERT_TRUE(mmapped_data);
  ASSERT_FALSE(init_mmapped_data);
  ASSERT_EQ(mmapped_data.size(), pnm::utils::byte_size_of_container(data));
  const auto view = mmapped_data.get_view<DataType>();
  const std::vector<DataType> read_vec(view.begin(), view.end());
  ASSERT_EQ(read_vec.size(), data.size());
  ASSERT_EQ(read_vec, data);
}

struct TestMmapedStruct {
  float float_field;
  double double_field;
  int int_field;
  bool bool_field;
  uint32_t uint32_t_field;
};

bool operator==(const TestMmapedStruct &lhs, const TestMmapedStruct &rhs) {
  return lhs.float_field == rhs.float_field &&
         lhs.double_field == rhs.double_field &&
         lhs.int_field == rhs.int_field && lhs.bool_field == rhs.bool_field &&
         lhs.uint32_t_field == rhs.uint32_t_field;
}

} // namespace

struct MmappedFileTest : public ::testing::Test {
  std::string root;
  std::filesystem::path file_path;

  void SetUp() override {
    root = (std::filesystem::temp_directory_path() / "embXXXXXX").string();
    ASSERT_TRUE(mkdtemp(root.data()));
    file_path = std::filesystem::path(root) / "random_vec";
  }

  void TearDown() override { std::filesystem::remove_all(root); }
};

TEST_F(MmappedFileTest, CheckUint8t) {
  test_mmapped_file<uint8_t>(file_path, generate_random_vector<uint8_t>(1000));
}

TEST_F(MmappedFileTest, CheckUint32t) {
  test_mmapped_file<uint32_t>(file_path,
                              generate_random_vector<uint32_t>(1000));
}

TEST_F(MmappedFileTest, CheckUint64t) {
  test_mmapped_file<uint64_t>(file_path,
                              generate_random_vector<uint64_t>(1000));
}

TEST_F(MmappedFileTest, CheckInt) {
  test_mmapped_file<int>(file_path, generate_random_vector<int>(1000));
}

TEST_F(MmappedFileTest, CheckFloat) {
  test_mmapped_file<float>(file_path, generate_random_vector<float>(1000));
}

TEST_F(MmappedFileTest, CheckDouble) {
  test_mmapped_file<double>(file_path, generate_random_vector<double>(1000));
}

TEST_F(MmappedFileTest, CheckStruct) {
  std::vector<TestMmapedStruct> data(1000);
  for (size_t i = 0UL; i < data.size(); ++i) {
    auto &item = data[i];
    item.bool_field = i % 2 == 0;
    item.int_field = i << 1;
    item.uint32_t_field = i << 2;
    item.float_field = i / 1.2345;
    item.double_field = i / 2.3456;
  }
  test_mmapped_file<TestMmapedStruct>(file_path, data);
}
