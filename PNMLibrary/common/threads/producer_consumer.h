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

#ifndef _PRODUCER_CONSUMER_H_
#define _PRODUCER_CONSUMER_H_

#include "common/threads/action_queue.h"

#include <future>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

namespace pnm::threads {

/*! Producer/consumer model to perform single async SLS run on device
 */
template <typename ThreadCreationStrategy> class ProducerConsumer {
public:
  ProducerConsumer(unsigned thread_count = 1,
                   ThreadCreationStrategy thread_creator = {},
                   const std::string &thread_name = {})
      : thread_creator_(std::move(thread_creator)),
        queue_{std::make_shared<ActionQueue>()} {
    // Create consumers
    for (; thread_count > 0; --thread_count) {
      thread_creator_.create(queue_, thread_name);
    }
  }

  ProducerConsumer(const ProducerConsumer &other) = delete;
  ProducerConsumer(ProducerConsumer &&other) noexcept
      : thread_creator_{std::move(other.thread_creator_)},
        queue_{std::move(other.queue_)} {}

  /* We don't want to close queue during the copy or move-assign */
  ProducerConsumer &operator=(const ProducerConsumer &other) = delete;
  ProducerConsumer &operator=(ProducerConsumer &&other) noexcept = delete;

  template <typename Func, typename... Args>
  std::future<void> run(Func &&func, Args &&...args) {
    Action action(
        [func = std::forward<Func>(func),
         args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
          // std::packaged_task catches any exceptions if they happen
          std::apply(std::forward<Func>(func), std::move(args));
        });

    auto future = action.get_future();

    queue_->push(std::move(action));

    return future;
  }

  ~ProducerConsumer() {
    if (queue_ != nullptr) {
      queue_->close();
    }
  }

private:
  ThreadCreationStrategy thread_creator_;
  // The main owner is ActionConsumer (when main thread was stopped)
  std::shared_ptr<ActionQueue> queue_;
};
} // namespace pnm::threads

#endif //_PRODUCER_CONSUMER_H_
