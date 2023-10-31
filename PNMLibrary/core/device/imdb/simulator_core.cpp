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

#include "core/device/imdb/simulator_core.h"

#include "imdb/software_scan/avx2/scan.h"

#include "core/device/imdb/reg_mem.h"
#include "core/device/simulator/base_simulator.h"

#include "common/profile.h"
#include "common/topology_constants.h"

#include "pnmlib/imdb/bit_containers.h"
#include "pnmlib/imdb/libimdb.h"
#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/misc_utils.h"
#include "pnmlib/common/views.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <thread>

namespace pnm::imdb::device {

using pnm::imdb::device::topo;

SimulatorCore::SimulatorCore(const RegisterPointers &regs, uint8_t *memory)
    : pnm::sls::device::BaseAsyncSimulator(topo().NumOfThreads),
      memory_{memory}, regs_{regs} {
  // 0b111 for 3 threads.
  regs_.status->IMDB_COM_STATUS = (1U << topo().NumOfThreads) - 1;

  // Set identification registers (values taken from the hardware device).
  //
  // Accel type: Column scan (0)
  // Major version: IMDB 3 threads (1)
  // Minor version: Initial (1)
  regs_.common->IMDB_ID = 0b0000'0001'0001;
  // Number of threads: 3
  // Number of cores: 1
  // Memory capacity: 0 (probably, not implemented in hardware yet?)
  regs_.common->IMDB_CAP = 0b11'01'00000000'00000000;
  pnm::sls::device::BaseAsyncSimulator::start();
}

SimulatorCore::~SimulatorCore() { stop(); }

void SimulatorCore::worker_job(size_t thread_id) {
  ClockTimerType clock_timer;
  clock_timer.tick();

  auto &own_regs = *regs_.thread[thread_id];
  while (!should_stop(thread_id)) {
    // Continue polling until ready to start the scan operation.
    // Each time the scan operation does not start, return to the beginning of
    // the loop to check the stop flag.
    {
      const auto ctrl_value =
          utils::as_vatomic<uint32_t>(&own_regs.THREAD_CTRL)->load();
      const bool should_start_scan =
          (ctrl_value & THREAD_CTRL_STATUS_MASK) != 0;
      if (!should_start_scan) {
        std::this_thread::yield();
        continue;
      }
    }

    // Scan operation has started.

    set_imdb_com_status(thread_id, ThreadStatus::Working);
    auto *atomic_status = utils::as_vatomic<uint32_t>(&own_regs.THREAD_STATUS);
    atomic_status->store(THREAD_STATUS_BUSY);

    // Collect common parameters.
    const bool is_index_vec =
        (own_regs.THREAD_CTRL & THREAD_CTRL_OUTPUT_TYPE_MASK) == THREAD_CTRL_IV;
    const bool is_in_list =
        (own_regs.THREAD_COMP_MODE & THREAD_COMP_MODE_OPERATION_TYPE_MASK) ==
        THREAD_COMP_MODE_IN_LIST;

    const auto output_type = is_index_vec ? pnm::imdb::OutputType::IndexVector
                                          : pnm::imdb::OutputType::BitVector;

    const auto operation_type = is_in_list ? pnm::imdb::OperationType::InList
                                           : pnm::imdb::OperationType::InRange;

    const uint64_t bit_size =
        own_regs.THREAD_COMP_MODE & THREAD_COMP_MODE_ENTRY_BIT_SIZE_MASK;

    // No point in supporting this
    assert(own_regs.THREAD_COLUMN_OFFSET == 0);
    const auto column_addr = own_regs.THREAD_START_ADDR;
    const auto column_size = own_regs.THREAD_SIZE_SB;

    const auto *column_elems =
        virtual_address<const pnm::imdb::compressed_element_type *>(
            column_addr);

    const auto column_view = pnm::containers::compressed_view_from_raw(
        bit_size, column_elems, column_size);

    const auto common_params = CommonParams{
        .column_view = column_view,
        .column_result_size_ref = own_regs.THREAD_RES_SIZE_SB,
    };

    const auto thread_context = ThreadContext{
        .clock_timer = clock_timer,
        .thread_regs = own_regs,
    };

    dispatch_scan_operation(output_type, operation_type, common_params,
                            thread_context);

    own_regs.THREAD_ERROR = 0;
    own_regs.THREAD_CTRL = 0;
    set_imdb_com_status(thread_id, ThreadStatus::Idle);
    atomic_status->store(0);
  }
}

// We don't make this function const because it changes the registers.
// NOLINTNEXTLINE(readability-make-member-function-const)
void SimulatorCore::set_imdb_com_status(size_t thread_id, ThreadStatus status) {
  auto *atomic_reg_ptr =
      utils::as_vatomic<uint32_t>(&regs_.status->IMDB_COM_STATUS);

  const uint32_t mask = 1U << static_cast<uint32_t>(thread_id);

  switch (status) {
  case ThreadStatus::Idle:
    atomic_reg_ptr->fetch_or(mask);
    return;
  case ThreadStatus::Working:
    atomic_reg_ptr->fetch_and(~mask);
    return;
  }
  throw pnm::error::InvalidArguments("Thread status.");
}

void SimulatorCore::dispatch_scan_operation(
    pnm::imdb::OutputType output_type, pnm::imdb::OperationType operation_type,
    const CommonParams &common_params, const ThreadContext &thread_context) {
  // Multiple dispatch by `output_type` and `operation_type`.
  // Dispatch pipeline:
  //
  //   dispatch_by_operation_type(): determine predicate
  //    -> dispatch_by_output_type(predicate): determine output_view
  //        -> invoke_scan(predicate, output_view): invoke scan operation
  //
  auto invoke_scan = [&](const auto &predicate, const auto &output_view) {
    auto &clock = thread_context.clock_timer;
    clock.tock();
    thread_context.thread_regs.THREAD_START_TIME = clock.duration().count();

    auto result_size = pnm::imdb::internal::avx2::scan(
        common_params.column_view, predicate, output_view);

    clock.tock();
    thread_context.thread_regs.THREAD_DONE_TIME = clock.duration().count();
    return result_size;
  };

  auto dispatch_by_output_type = [&](const auto &predicate) {
    switch (output_type) {
    case pnm::imdb::OutputType::BitVector: {
      auto *column_result_addr = virtual_address<pnm::imdb::index_type *>(
          thread_context.thread_regs.THREAD_RES_ADDR);
      auto bit_vector_view = pnm::containers::bit_view_from_raw(
          column_result_addr, common_params.column_view.size());

      invoke_scan(predicate, bit_vector_view);
      // Bit-vector versions of AVX2 scan return an irrelevant element count.
      // Count the number of matching elements ourselves.
      auto result_size = thread_context.thread_regs.THREAD_SIZE_SB;
      common_params.column_result_size_ref = static_cast<uint64_t>(result_size);
      break;
    }
    case pnm::imdb::OutputType::IndexVector: {
      auto *column_result_addr = virtual_address<pnm::imdb::index_type *>(
          thread_context.thread_regs.THREAD_RES_ADDR +
          thread_context.thread_regs.THREAD_RES_INDEX_OFFSET);
      auto index_vector_view = pnm::views::make_view(
          column_result_addr, thread_context.thread_regs.THREAD_RSLT_LMT);

      auto result_size = invoke_scan(predicate, index_vector_view);
      common_params.column_result_size_ref = static_cast<uint64_t>(result_size);
      break;
    }
    }
  };

  auto dispatch_by_operation_type = [&]() {
    switch (operation_type) {
    case pnm::imdb::OperationType::InList: {
      auto list_addr_raw = thread_context.thread_regs.THREAD_INLIST_ADDR;
      auto list_size = thread_context.thread_regs.THREAD_INLIST_SIZE;
      const auto *list_addr =
          virtual_address<const pnm::imdb::compressed_element_type *>(
              list_addr_raw);

      auto in_list_predicate =
          pnm::containers::bit_view_from_raw(list_addr, list_size);
      dispatch_by_output_type(in_list_predicate);
      break;
    }
    case pnm::imdb::OperationType::InRange: {
      auto min_value = thread_context.thread_regs.THREAD_MIN_VALUE;
      auto max_value = thread_context.thread_regs.THREAD_MAX_VALUE;

      auto in_range_predicate = pnm::imdb::RangeOperation{min_value, max_value};
      dispatch_by_output_type(in_range_predicate);
      break;
    }
    }
  };

  dispatch_by_operation_type();
}

} // namespace pnm::imdb::device
