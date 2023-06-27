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

#include "common/threads/action_consumer.h"
#include "common/threads/action_queue.h"
#include "common/threads/producer_consumer.h"
#include "common/threads/workers_storage.h"

#include <gtest/gtest.h>

#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <future>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace {
struct InplacePlus {
  void operator()(int A, int &B, int *C) const { *C = A + B; }
};
class ThreadManager {
public:
  void create(const std::shared_ptr<pnm::threads::ActionQueue> &queue,
              const std::string & /*thread_name*/) {
    workers_storage_.add_worker([queue]() {
      pnm::threads::ActionConsumer ae(queue);
      ae.start();
    });
  }

private:
  pnm::threads::Storage workers_storage_;
};
} // namespace

TEST(AsyncRunner, CreateNoUseClose) {
  std::thread th;
  struct SpecialThreadManager {
    std::thread &th;
    void create(const std::shared_ptr<pnm::threads::ActionQueue> &queue,
                const std::string & /*thread_name*/) {
      th = std::thread([queue]() {
        pnm::threads::ActionConsumer ae(queue);
        ae.start();
      });
    }
  };
  {
    pnm::threads::ProducerConsumer<SpecialThreadManager>(
        1, SpecialThreadManager{th});
    static constexpr auto SOME_DELAY = 100;
    std::this_thread::sleep_for(std::chrono::milliseconds(SOME_DELAY));
  }
  ASSERT_NO_THROW(th.join());
}

void test_function_impl() {
  std::vector<int> A(100000), B(100000), C(100000, 0);
  std::default_random_engine eng{std::random_device{}()};
  std::uniform_int_distribution<int> uni{0, 100};
  std::generate(A.begin(), A.end(), [&uni, &eng]() { return uni(eng); });
  std::generate(B.begin(), B.end(), [&uni, &eng]() { return uni(eng); });

  std::vector<int> golden(A.size(), 0);
  std::transform(A.begin(), A.end(), B.begin(), golden.begin(), std::plus{});

  auto runner = pnm::threads::ProducerConsumer(1, ThreadManager{});

  const InplacePlus plus;

  for (auto i = 0U; i < A.size(); ++i) {
    runner.run(plus, A[i], std::ref(B[i]), &C[i]).get();
  }

  ASSERT_TRUE(std::equal(C.begin(), C.end(), golden.begin()));
}

TEST(AsyncRunner, AsyncVectorAdd) { test_function_impl(); }

TEST(AsyncRunner, MultiThreadAsyncVectorAdd) {
  auto th = std::async(std::launch::async, test_function_impl);
  auto th1 = std::async(std::launch::async, test_function_impl);
  auto th2 = std::async(std::launch::async, test_function_impl);
  ASSERT_NO_THROW((th.get(), th1.get(), th2.get()));
}

TEST(AsyncRunner, MultiProcessAsyncVectorAdd) {
  if (fork() == 0) {
    test_function_impl();
    std::exit(::testing::Test::HasFailure());
  } else {
    if (fork() == 0) {
      test_function_impl();
      std::exit(::testing::Test::HasFailure());
    } else {
      test_function_impl();
    }
  }
}

TEST(AsyncRunner, MultipleProducerMultipleConsumer) {
  std::vector<int> A(100000), B(100000), C(100000, 0);
  std::default_random_engine eng{std::random_device{}()};
  std::uniform_int_distribution<int> uni{0, 100};
  std::generate(A.begin(), A.end(), [&uni, &eng]() { return uni(eng); });
  std::generate(B.begin(), B.end(), [&uni, &eng]() { return uni(eng); });

  std::vector<int> golden(A.size(), 0);
  std::transform(A.begin(), A.end(), B.begin(), golden.begin(), std::plus{});

  auto process_range = [&A, &B, &C](auto start, auto end) {
    // create runners at heap to return them
    // this allows to close action queue after all jobs complete
    auto *runner = new pnm::threads::ProducerConsumer(1, ThreadManager{});
    const InplacePlus plus;
    for (auto i = start; i < end; ++i) {
      runner->run(plus, A[i], std::ref(B[i]), &C[i]).get();
    }
    return runner;
  };
  static constexpr auto NUMBER_OF_CONSUMERS = 4;
  std::array<std::future<pnm::threads::ProducerConsumer<ThreadManager> *>,
             NUMBER_OF_CONSUMERS>
      workers;
  auto data_per_worker = A.size() / NUMBER_OF_CONSUMERS;
  auto current_start = 0U;
  std::generate(workers.begin(), workers.end(),
                [&process_range, data_per_worker, &current_start]() {
                  auto worker = std::async(std::launch::async, process_range,
                                           current_start,
                                           current_start + data_per_worker);
                  current_start += data_per_worker;
                  return worker;
                });
  std::vector<pnm::threads::ProducerConsumer<ThreadManager> *> wptrs(
      workers.size());

  // Wait for all jobs done, copy consumers ptr.
  std::transform(workers.begin(), workers.end(), wptrs.begin(),
                 [](auto &w) { return w.get(); });

  // Destroy consumers, close queue
  std::for_each(wptrs.begin(), wptrs.end(), [](auto ptr) { delete ptr; });

  ASSERT_TRUE(std::equal(C.begin(), C.end(), golden.begin()));
}
