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

#include "core/device/simulator/base_simulator.h"

#include "common/make_error.h"
#include "common/threads/producer_consumer.h"

#include <atomic>
#include <cassert>
#include <cstddef>
#include <future>
#include <memory>

namespace pnm::sls::device {

BaseAsyncSimulator::BaseAsyncSimulator(size_t threads_num)
    : threads_num_(threads_num) {
  assert(threads_num_ > 0);
  thread_controls_ = std::make_unique<ThreadControls[]>(threads_num_);
}

BaseAsyncSimulator::~BaseAsyncSimulator() { assert(!thread_pool_); }

void BaseAsyncSimulator::start() {
  assert(!thread_pool_);
  thread_pool_ = std::make_unique<ThreadPoolType>(threads_num_);
}

void BaseAsyncSimulator::stop() {
  for (size_t thread_id = 0; thread_id < threads_num_; ++thread_id) {
    if (is_thread_enabled(thread_id)) {
      disable_thread(thread_id);
    }
  }
  thread_pool_.reset();
}

bool BaseAsyncSimulator::should_stop(size_t thread_id) const {
  return thread_controls_[thread_id].stop_flag.load();
}

void BaseAsyncSimulator::enable_thread(size_t thread_id) {
  assert(thread_pool_);
  if (is_thread_enabled(thread_id)) {
    throw pnm::error::make_inval("Trying to enable already enabled thread {}.",
                                 thread_id);
  }
  auto &controls = thread_controls_[thread_id];
  controls.stop_flag.store(false);
  controls.finished =
      thread_pool_->run([this, thread_id]() { worker_job(thread_id); });
}

void BaseAsyncSimulator::disable_thread(size_t thread_id) {
  if (!is_thread_enabled(thread_id)) {
    throw pnm::error::make_inval(
        "Trying to disable thread {} that is not enabled.", thread_id);
  }

  auto &controls = thread_controls_[thread_id];
  controls.stop_flag.store(true);

  controls.finished.get();
}

bool BaseAsyncSimulator::is_thread_enabled(size_t thread_id) const {
  if (thread_id >= threads_num_) {
    throw pnm::error::make_inval("Simulator thread id {} is too large.",
                                 thread_id);
  }

  return thread_controls_[thread_id].finished.valid();
}

} // namespace pnm::sls::device
