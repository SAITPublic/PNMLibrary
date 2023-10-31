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

#include "pnmlib/core/buffer.h"

#include "test/utils/pnm_fmt.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <fmt/core.h>
#include <fmt/format.h>

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <utility>
#include <vector>

class Buffer : public testing::TestWithParam<pnm::Device::Type> {};

INSTANTIATE_TEST_SUITE_P(BufferParamSuite, Buffer,
                         ::testing::Values(pnm::Device::Type::IMDB,
                                           pnm::Device::Type::SLS),
                         print_fmt_test_params);

TEST_P(Buffer, Create) {
  auto context = pnm::make_context(GetParam());
  static constexpr auto SIZE = 42;
  pnm::memory::Buffer<uint32_t> no_bind_buffer(SIZE, context);
  ASSERT_EQ(no_bind_buffer.size(), SIZE);

  ASSERT_FALSE(no_bind_buffer.is_offloaded());
  ASSERT_TRUE(no_bind_buffer.is_device_region_owned());

  ASSERT_THROW(no_bind_buffer.copy_to_device(), pnm::error::InvalidArguments);
  ASSERT_THROW(no_bind_buffer.copy_from_device(), pnm::error::InvalidArguments);

  std::vector input{4, 8, 28, 42};

  pnm::memory::Buffer bound_buffer(pnm::views::make_view(input), context);
  ASSERT_EQ(bound_buffer.size(), input.size());

  ASSERT_TRUE(bound_buffer.is_offloaded());
  ASSERT_TRUE(bound_buffer.is_device_region_owned());

  ASSERT_NO_THROW(EXPECT_EQ(bound_buffer.copy_to_device(), input.size()));

  auto accessor = bound_buffer.direct_access();
  fmt::print("{}\n", fmt::join(accessor, ", "));

  ASSERT_TRUE(std::equal(accessor.begin(), accessor.end(), input.begin()));

  {
    pnm::memory::Buffer<int> view_buffer(bound_buffer.device_region(), context);
    ASSERT_FALSE(view_buffer.is_device_region_owned());
    auto view_buffer_accessor = view_buffer.direct_access();
    ASSERT_TRUE(
        std::equal(input.begin(), input.end(), view_buffer_accessor.begin()));
  }

  auto op = [](auto &e) { e = e * 2; };

  auto golden = input;
  std::for_each(golden.begin(), golden.end(), op);

  for (auto e : accessor) {
    op(e);
  }

  ASSERT_NO_THROW(EXPECT_EQ(bound_buffer.copy_from_device(), input.size()));
  fmt::print("{}\n", fmt::join(input, ", "));

  ASSERT_EQ(input, golden);
}

TEST_P(Buffer, Sls) {
  auto context = pnm::make_context(pnm::Device::Type::SLS);

  std::vector input{4, 8, 28, 42};
  pnm::memory::Buffer bound_buffer(pnm::views::make_view(input), context);
  ASSERT_EQ(bound_buffer.size(), input.size());
  ASSERT_NO_THROW(
      std::get<pnm::memory::RankedRegion>(bound_buffer.device_region()));
  ASSERT_TRUE(bound_buffer.is_device_region_owned());

  {
    pnm::memory::Buffer<int> view_buffer(bound_buffer.device_region(), context);
    ASSERT_FALSE(view_buffer.is_device_region_owned());
    auto view_buffer_accessor = view_buffer.direct_access();
    ASSERT_TRUE(
        std::equal(input.begin(), input.end(), view_buffer_accessor.begin()));
  }

  auto bound_buffer_accessor = bound_buffer.direct_access();
  ASSERT_TRUE(
      std::equal(input.begin(), input.end(), bound_buffer_accessor.begin()));
}

TEST_P(Buffer, BindUserRange) {
  auto ctx = pnm::make_context(pnm::Device::Type::IMDB);
  static constexpr auto N = 1024;
  pnm::memory::Buffer<uint64_t> buffer(N, ctx);

  auto acc = buffer.direct_access();
  std::iota(acc.begin(), acc.end(), 10);

  std::vector<uint64_t> user_space(10);
  ASSERT_THROW(buffer.bind_user_region(pnm::views::make_view(user_space)),
               pnm::error::InvalidArguments);
  user_space.resize(1500);
  ASSERT_THROW(buffer.bind_user_region(pnm::views::make_view(user_space)),
               pnm::error::InvalidArguments);

  user_space.resize(buffer.size());
  ASSERT_NO_THROW(buffer.bind_user_region(pnm::views::make_view(user_space)));

  ASSERT_EQ(buffer.copy_from_device(), buffer.size());

  for (auto i = 0UL, k = 10UL; i < user_space.size(); ++i, ++k) {
    ASSERT_EQ(user_space[i], k);
  }
}

TEST_P(Buffer, MoveOperation) {
  auto context = pnm::make_context(pnm::Device::Type::IMDB);
  static constexpr char msg[] = "Lazy brown fox jumps over the huge dog.";
  pnm::memory::Buffer<char> buff(sizeof(msg), context);
  auto acc = buff.direct_access();
  std::copy(msg, msg + sizeof(msg), acc.begin());

  pnm::memory::Buffer mvc_buff(std::move(buff));

  ASSERT_EQ(mvc_buff.size(), sizeof(msg));
  ASSERT_EQ(mvc_buff.context(), context.get());
  acc = mvc_buff.direct_access();
  ASSERT_TRUE(std::equal(acc.begin(), acc.end(), msg));

  fmt::print("{}\n", fmt::join(acc, ""));

  pnm::memory::Buffer<char> replaced(102, context);
  replaced = std::move(mvc_buff);

  ASSERT_EQ(replaced.size(), sizeof(msg));
  ASSERT_EQ(replaced.context(), context.get());
  acc = replaced.direct_access();
  ASSERT_TRUE(std::equal(acc.begin(), acc.end(), msg));

  fmt::print("{}\n", fmt::join(acc, ""));
}
