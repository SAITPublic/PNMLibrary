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

#ifndef _ACTION_QUEUES_H_
#define _ACTION_QUEUES_H_

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <utility>

namespace pnm::threads {
using Action = std::packaged_task<void()>;

/*! Class to handle actions queue and provide thread safe access to it */
class ActionQueue {
public:
  void push(Action action) {
    const std::lock_guard lg(queue_mutex_);
    actions_.emplace(std::move(action));

    queue_cv_.notify_all();
  }

  Action pop() {
    std::unique_lock lg(queue_mutex_);

    // Wait until queue has elements or queue was closed
    queue_cv_.wait(lg, [this]() { return !(actions_.empty() && is_active_); });

    if (!is_active_ && actions_.empty()) {
      return {};
    }

    auto action = std::move(actions_.front());
    actions_.pop();
    return action;
  }

  void close() {
    { // Atomic is not suitable here. Atomic can be changed before wait is
      // start, but after the condition check
      const std::lock_guard lg(queue_mutex_);
      is_active_ = false;
    }
    queue_cv_.notify_all();
  }

  void reopen() {
    const std::lock_guard lg(queue_mutex_);
    is_active_ = true;
  }

  ~ActionQueue() { close(); }

private:
  std::queue<Action> actions_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  bool is_active_ = true;
};
} // namespace pnm::threads

#endif //_ACTION_QUEUES_H_
