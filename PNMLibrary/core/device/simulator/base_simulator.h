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

#ifndef _SLS_ASYNC_SIMULATOR_CORE_H_
#define _SLS_ASYNC_SIMULATOR_CORE_H_

#include "common/compiler_internal.h"
#include "common/threads/producer_consumer.h"
#include "common/threads/workers_storage.h"

#include <atomic>
#include <cstddef>
#include <future>
#include <memory>

namespace pnm::sls::device {

/** @brief The base class for async SLS and IMDB simulators.
 *
 * Provides thread pool and methods to start/stop worker threads.
 * To execute particular worker specific to concrete simulator implementation
 * inheritance is used.
 */
class BaseAsyncSimulator {
public:
  explicit BaseAsyncSimulator(size_t threads_num);
  virtual ~BaseAsyncSimulator();

  /** @brief Enable simulated thread.
   *
   * The simulated thread with specified id becomes active and
   * starts polling per-thread registers. In order to do this, it
   * allocates a vacant thread from an internal thread pool and runs on it.
   * The thread continues running until deactivated with `disable_thread()`.
   *
   * This function must only be called if enabling the specified thread would
   * not cause any race conditions.
   *
   * Does not block, returns immediately.
   *
   * @throws pnm::error::InvalidArguments if the specified thread is already
   * enabled or if the thread id is invalid.
   */
  void enable_thread(size_t thread_id);

  /** @brief Stop and disable a simulated thread.
   *
   * The simulated thread is stopped gracefully, waiting for completion of
   * the scan operation in progress, if any.
   *
   * @throws pnm::error::InvalidArguments if the specified thread is disabled or
   * if the thread id is invalid.
   */
  void disable_thread(size_t thread_id);

  /** @brief Check if the simulated thread is enabled.
   *
   * @returns true if the thread is enabled, false otherwise.
   *
   * @throws pnm::error::InvalidArguments if the thread id is invalid.
   */
  bool is_thread_enabled(size_t thread_id) const;

  /** @brief Check if simulator thread should exit.
   *
   * @returns true if simulator was stopped.
   */
  bool should_stop(size_t thread_id) const;

  /** @brief Start simulator thread pool.
   */
  void start();

  /** @brief Stop simulator thread pool.
   */
  void stop();

protected:
  virtual void worker_job(size_t thread_id) = 0;

  struct CACHE_ALIGNED ThreadControls {
    /** Flag that signals the thread to stop. */
    std::atomic_bool stop_flag;
    /** Future that completes when the thread actually stops */
    std::future<void> finished;
  };

  using ThreadPoolType = pnm::threads::ProducerConsumer<pnm::threads::Manager>;
  std::unique_ptr<ThreadPoolType> thread_pool_;
  std::unique_ptr<ThreadControls[]> thread_controls_;
  size_t threads_num_;
};

} // namespace pnm::sls::device

#endif
