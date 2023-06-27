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

#ifndef _IMDB_SIMULATOR_CORE_H_
#define _IMDB_SIMULATOR_CORE_H_

#include "reg_mem.h"

#include "common/compiler_internal.h"
#include "common/profile.h"
#include "common/threads/producer_consumer.h"
#include "common/threads/workers_storage.h"

#include "pnmlib/imdb/bit_views.h"
#include "pnmlib/imdb/libimdb.h"
#include "pnmlib/imdb/scan_types.h"

#include <linux/imdb_resources.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <future>
#include <optional>

namespace pnm::imdb::device {

namespace detail {
/** @brief Wrapper around `std::atomic_bool` aligned to be friendly with CPU
 * cache.
 *
 * The alignment is done by `hardware_destructive_interference_size`, which
 * ensures that cache conflicts do not happen for atomic operations on
 * neighboring values.
 */
struct AlignedAtomicBool {
  CACHE_ALIGNED std::atomic<bool> inner;
};
} // namespace detail

/** @brief The core of the IMDB device simulator
 *
 * Simulates engine threads and register I/O handling as a freestanding
 * functional module.
 *
 * IMDB register memory should be allocated or obtained by the user of this
 * class, and pointers to this memory are to be provided in the constructor.
 * This adds flexibility to the usage, because it allows for the registers to
 * reside in any already allocated memory area.
 *
 * The clock timer is simulated per-thread using the PNM profiling timer
 * with one tick being equivalent to one millisecond.
 *
 * [TODO: @a.korzun] Adjust the clock tick length according to the
 * specification.
 *
 * It is UB to modify any global or thread registers while this thread is
 * executing a scan operation. The `THREAD_CTRL` registers must be written to
 * using atomic stores. All register modifications must happen prior to the
 * write to `THREAD_CTRL` that starts a scan operation or after the scan
 * operation has finished (as reflected in `THREAD_STATUS` or
 * `IMDB_COM_STATUS`).
 *
 * Due to the non-realtime nature of the OS scheduler, it is impossible to
 * update the status registers immediately or with a predictable delay
 * after writing to the `THREAD_CTRL` register to start the scan operation.
 * It might thus not be possible to tell apart the cases when an engine has
 * not yet started operation execution and when it has finished it by reading
 * status registers (`IMDB_COM_STATUS` and `THREAD_STATUS`). This problem is
 * only present on a simulator and is irrelevant with a hardware device. It is
 * recommended to zero the `THREAD_STATUS` register before writing to
 * `THREAD_CTRL` only on a simulator (on hardware, this register is read-only).
 * This way, `THREAD_STATUS` becomes non-zero only when the scan operation
 * completes.
 *
 * The simulator starts with all engine threads disabled to avoid race
 * conditions when multiple simulators operate on the same register memory at
 * the same time. See the `enable_thread()` and `disable_thread()` methods for
 * additional information.
 */
class SimulatorCore {
public:
  SimulatorCore(const RegisterPointers &regs, uint8_t *memory);

  /**
   * @brief Destructor.
   *
   * Stops all the threads that are currently waiting for a scan operation.
   */
  ~SimulatorCore();

  /** @brief Enable a simulated IMDB thread.
   *
   * The simulated IMDB thread with specified id becomes active and
   * starts polling per-thread registers. In order to do this, it
   * allocates a vacant thread from an internal thread pool and runs on it.
   * The thread continues running until deactivated with `disable_thread()`.
   *
   * This function must only be called if enabling the specified thread would
   * not cause any race conditions. If system-wide register memory is used,
   * (e.g. simulated or hardware register memory), this means the current
   * application thread must hold a lock on the corresponding IMDB thread
   * obtained from the IMDB resource manager. The lock must not be released
   * until this simulator thread is disabled.
   *
   * Does not block, returns immediately.
   *
   * @throws pnm::error::InvalidArguments if the specified thread is already
   * enabled or if the thread id is invalid.
   */
  void enable_thread(size_t thread_id);

  /** @brief Stop and disable a simulated IMDB thread.
   *
   * The simulated IMDB thread is stopped gracefully, waiting for completion of
   * the scan operation in progress, if any. After being stopped, this simulated
   * engine thread would no longer access IMDB registers until re-enabled.
   *
   * This function blocks until the simulated engine thread is fully stopped. It
   * is thus safe to release any lock on the IMDB thread after this function
   * returns.
   *
   * @throws pnm::error::InvalidArguments if the specified thread is disabled or
   * if the thread id is invalid.
   */
  void disable_thread(size_t thread_id);

  /** @brief Check if the simulated IMDB thread is enabled.
   *
   * @returns true if the thread is enabled, false otherwise.
   *
   * @throws pnm::error::InvalidArguments if the thread id is invalid.
   */
  bool is_thread_enabled(size_t thread_id) const;

private:
  using ClockTimerType = pnm::profile::TimerMsT;

  /** @brief Status of an engine thread. */
  enum class ThreadStatus {
    /** The thread is busy and is executing a scan operation. */
    Working = 0,
    /** The thread is idle and is waiting for tasks. */
    Idle = 1,
  };

  /** @brief Engine-specific context passed to thread-agnostic methods. */
  struct ThreadContext {
    /** Per-thread clock timer. */
    ClockTimerType &clock_timer;
    /** Per-thread register set. */
    volatile ThreadCSR &thread_regs;
  };

  /** @brief Common parameters for all scan operations. */
  struct CommonParams {
    /** The input data column view. */
    pnm::views::bit_compressed_view<const pnm::imdb::compressed_element_type>
        column_view;
    /** Reference to the output column size. */
    volatile uint64_t &column_result_size_ref;
  };

  /** @brief Controls to interact with worker threads. */
  struct ThreadControls {
    /** Flag that signals the thread to stop. */
    detail::AlignedAtomicBool stop_flag;
    /** Future that completes when the thread actually stops, if the thread is
     * enabled. `std::nullopt` if the thread is disabled. */
    std::optional<std::future<void>> future;
  };

  /** @brief Worker thread entry point. */
  void worker_job(size_t thread_id);

  /** @brief Set the thread status in the `IMDB_COM_STATUS` register. */
  void set_imdb_com_status(size_t thread_id, ThreadStatus status);

  /** @brief Evaluate virtual address for the specified offset. */
  template <typename T> T virtual_address(uintptr_t offset) const {
    return std::launder(reinterpret_cast<T>(memory_ + offset));
  }

  /** @brief Invoke the scan operation of requested type.
   *
   * Thread-specific information is provided through `thread_context`.
   * Scan operation parameters specific to the provided output or operation
   * type will be read from the thread registers.
   *
   * @param output_type scan output type (IV/BV), selects an implementation
   * @param operation_type scan mode (in list/in range), selects an
   * implementation
   * @param common_params common parameters of any scan operation
   * @param thread_context thread-specific context
   */
  void dispatch_scan_operation(pnm::imdb::OutputType output_type,
                               pnm::imdb::OperationType operation_type,
                               const CommonParams &common_params,
                               const ThreadContext &thread_context);

  /** Thread controls for each simulated IMDB thread. */
  std::array<ThreadControls, IMDB_THREAD_NUM> thread_controls_;

  /** Pointer to IMDB memory */
  uint8_t *memory_;

  /** Pointers to IMDB registers. */
  RegisterPointers regs_;

  /** Thread pool for scan operations. */
  pnm::threads::ProducerConsumer<pnm::threads::Manager> thread_pool_;
};

} // namespace pnm::imdb::device

#endif // _IMDB_SIMULATOR_CORE_H_
