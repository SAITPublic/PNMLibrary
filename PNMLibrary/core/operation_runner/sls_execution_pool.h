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

#ifndef _SLS_EXECUTION_POOL_H_
#define _SLS_EXECUTION_POOL_H_

#include "common/log.h"
#include "common/threads/producer_consumer.h"
#include "common/threads/workers_storage.h"
#include "common/topology_constants.h"

#include <utility>

namespace pnm::sls {

/*! \brief Wrapper over producer/consumer model.
 *
 * Class handles producer/consumer object and provides method to push task to
 * action queue.
 *
 * In multithreading cases, all threads share the same object, and as a result
 * - same action queues and consumer threads.
 * */
class ExecutionPool {
public:
  ExecutionPool(const ExecutionPool &) = delete;
  ExecutionPool(ExecutionPool &&) noexcept = delete;
  auto operator=(const ExecutionPool &) = delete;
  auto operator=(ExecutionPool &&) noexcept = delete;
  ~ExecutionPool() = default;

  static ExecutionPool &get() {
    // Use thread-safe initialization of the static variable
    static ExecutionPool pool;
    return pool;
  }

  template <typename Func, typename... Args>
  auto run(Func &&func, Args &&...args) {
    return pc_.run(std::forward<Func>(func), std::forward<Args>(args)...);
  }

protected:
  ExecutionPool()
      : threads_count_{pnm::device::topo().NumOfRanks},
        pc_{threads_count_, {}, thread_prefix_} {
    pnm::log::debug("{}[{}]: Create pool with {} threads", __PRETTY_FUNCTION__,
                    __LINE__, threads_count_);
  }

private:
  static constexpr auto thread_prefix_ = "SlsRunThread";

  unsigned int threads_count_;
  pnm::threads::ProducerConsumer<pnm::threads::Manager> pc_;
};
} // namespace pnm::sls

#endif //_SLS_EXECUTION_POOL_H_
