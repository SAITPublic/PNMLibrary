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

#ifndef _WORKERS_STORAGE_H_
#define _WORKERS_STORAGE_H_

#include "common/threads/action_consumer.h"
#include "common/threads/action_queue.h"
#include "common/threads/name.h"

#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace pnm::threads {

class Storage {
public:
  template <typename Worker> void add_worker(Worker worker_func) {
    workers_.emplace_back(std::move(worker_func));
  }

  Storage() = default;
  Storage(Storage &&) noexcept = default;
  Storage(const Storage &) = delete;
  Storage &operator=(const Storage &) = delete;
  ~Storage() { join_workers(); }

private:
  void join_workers() {
    for (auto &worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }

  std::vector<std::thread> workers_;
};

/*! \brief Create thread for producer/consumer model. Thread acquire an
 * ActionQueue and start listening it */
class Manager {
public:
  void create(std::shared_ptr<ActionQueue> queue,
              const std::string &thread_prefix = {}) {
    workers_storage_.add_worker(
        [q = std::move(queue), prefix = thread_prefix]() mutable {
          set_thread_name(std::move(prefix));
          ActionConsumer ae(std::move(q));
          ae.start();
        });
  }

private:
  Storage workers_storage_;
};

} // namespace pnm::threads

#endif
