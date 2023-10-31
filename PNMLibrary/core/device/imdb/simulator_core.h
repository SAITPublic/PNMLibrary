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

#include "core/device/simulator/base_simulator.h"

#include "common/profile.h"

#include "pnmlib/imdb/bit_views.h"
#include "pnmlib/imdb/libimdb.h"
#include "pnmlib/imdb/scan_types.h"

#include <cstddef>
#include <cstdint>

namespace pnm::imdb::device {
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
class SimulatorCore : public pnm::sls::device::BaseAsyncSimulator {
public:
  SimulatorCore(const RegisterPointers &regs, uint8_t *memory);

  /**
   * @brief Destructor.
   *
   * Stops all the threads that are currently waiting for a scan operation.
   */
  ~SimulatorCore() override;

private:
  using ClockTimerType = pnm::profile::TimerMsT;

  /** @brief Status of an engine thread. */
  enum class ThreadStatus : uint8_t {
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
    pnm::views::bit_compressed<const pnm::imdb::compressed_element_type>
        column_view;
    /** Reference to the output column size. */
    volatile uint64_t &column_result_size_ref;
  };

  /** @brief Worker thread entry point. */
  void worker_job(size_t thread_id) override;

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
  /** Pointer to IMDB memory */
  uint8_t *memory_;

  /** Pointers to IMDB registers. */
  RegisterPointers regs_;
};

} // namespace pnm::imdb::device

#endif // _IMDB_SIMULATOR_CORE_H_
