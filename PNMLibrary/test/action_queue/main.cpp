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

#include "common/threads/action_queue.h"

#include <gtest/gtest-typed-test.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-type-util.h>

#include <algorithm>
#include <future>
#include <memory>
#include <random>
#include <type_traits>
#include <vector>

using pnm::threads::Action;
using pnm::threads::ActionQueue;

TEST(ActionQueue, PushPopSync) {
  unsigned var1{}, var2{};
  auto queue = std::make_shared<ActionQueue>();

  queue->push(Action([&var1]() { var1 = 10; }));
  auto act = queue->pop();
  act();

  queue->push(Action([&var2]() { var2 = 5; }));
  act = queue->pop();
  act();

  EXPECT_EQ(var1, 10);
  EXPECT_EQ(var2, 5);

  queue->close();
  EXPECT_FALSE(queue->pop().valid());
}

template <typename NType> struct ActionQueueFixture : ::testing::Test {
  static constexpr auto NThreads = NType::value;
};
using ThreadNums = ::testing::Types<std::integral_constant<unsigned, 1>,
                                    std::integral_constant<int, 8>>;

TYPED_TEST_SUITE(ActionQueueFixture, ThreadNums);

void create_consumers(unsigned nthreads,
                      const std::shared_ptr<ActionQueue> &queue,
                      std::vector<std::future<void>> &consumers) {
  auto consumer = [queue]() {
    while (true) {
      auto func = queue->pop();

      if (!func.valid()) {
        break;
      }

      func();
    }
  };

  for (auto i = 0U; i < nthreads; ++i) {
    consumers.emplace_back(std::async(std::launch::async, consumer));
  }
}

TYPED_TEST(ActionQueueFixture, PushPopASync) {
  std::vector<int> data(10000);
  std::default_random_engine eng{std::random_device{}()};
  std::uniform_int_distribution<int> uni{0, 100};
  std::generate(data.begin(), data.end(), [&uni, &eng]() { return uni(eng); });

  auto origin = data;

  auto queue = std::make_shared<ActionQueue>();

  auto producer = [&data, queue]() {
    for (auto &e : data) {
      queue->push(Action([&e]() { e = -(e * e); }));
    }
  };

  auto producer_thread = std::async(std::launch::async, producer);
  std::vector<std::future<void>> consumers;
  create_consumers(TestFixture::NThreads, queue, consumers);

  producer_thread.get();
  queue->close();
  for (auto &c : consumers) {
    c.get();
  }

  ASSERT_TRUE(std::equal(origin.begin(), origin.end(), data.begin(),
                         [](auto lhs, auto rhs) { return lhs * lhs == -rhs; }));
}
