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

#include "core/operation_runner/sls_execution_pool.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <future>
#include <iterator>
#include <thread>
#include <vector>

void test_function() {
  auto &pool = pnm::sls::ExecutionPool::get();
  static constexpr auto N_WORKERS = 4;
  std::vector<std::vector<int>> result(N_WORKERS);
  auto test_function = [](std::vector<int> &out, int value) {
    std::fill_n(std::back_inserter(out), 100, value);

    // Explicit delay as a workload imitation
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  };

  std::future<void> futures[N_WORKERS];
  for (const auto i : {0, 1, 2, 3}) {
    futures[i] = pool.run(test_function, std::ref(result[i]), i + 1);
  }

  for (auto &f : futures) {
    f.wait();
  }

  for (auto i = 0; i < N_WORKERS; ++i) {
    ASSERT_EQ(std::find_if(result[i].begin(), result[i].end(),
                           [i](auto v) { return v != i + 1; }),
              result[i].end());
  }
}

TEST(ExecutionPool, SingleThread) { test_function(); }

TEST(ExecutionPoll, MultiThread) {
  auto th1 = std::thread(test_function);
  auto th2 = std::thread(test_function);

  th1.join();
  th2.join();

  auto th3 = std::thread(test_function);
  auto th4 = std::thread(test_function);
  th3.join();
  th4.join();
}
