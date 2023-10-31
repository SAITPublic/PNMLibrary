/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or
 * computer language in any form
 * by any means, electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung
 * Electronics.
 */

#ifndef PNM_REDUCTION_RUNNER_H
#define PNM_REDUCTION_RUNNER_H

#include "sls/compute_unit/compute_unit.h"
#include "sls/operation/reduction_operation.h"

#include "core/device/sls/base.h"
#include "core/device/sls/rank_memory.h"
#include "core/operation/internal.h"
#include "core/operation_runner/internal_runner.h"

#include "common/log.h"
#include "common/make_error.h"
#include "common/threads/producer_consumer.h"
#include "common/threads/workers_storage.h"

#include "pnmlib/core/device.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/views.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>

namespace pnm::sls {

using pnm::threads::ProducerConsumer;

template <typename Implementation>
class ReductionRunner : public InternalRunner {
protected:
  explicit ReductionRunner(Device *device, unsigned int num_of_workers)
      : num_of_workers_(num_of_workers) {
    if (device == nullptr) {
      throw pnm::error::InvalidArguments("Expected device.");
    }

    device_ = device->as<sls::device::BaseDevice>();
  }

  void run_operation(ReductionOperation &op) {
    sls::device::RankMemory::packs_array<std::future<void>> run_queue;
    size_t exec_queue = 0;

    // Store a pack for main thread. This allows to utilize all threads for
    // calculation and do not sleep at wait() in main thread all time
    uint8_t pack_for_main_thread = 0; // We don't have zero pack-id

    for (const auto pack : sls::device::RankMemory::pack_identifiers()) {
      if (op.get_num_exec_iterations(pack)) {
        if (!pack_for_main_thread) { // Create task for main thread
          pack_for_main_thread = pack;
        } else { // Push task to ActionQueue if main task exists
          run_queue[exec_queue] = pool_->run(
              &Implementation::run_on_pack, static_cast<Implementation *>(this),
              std::ref(op), pack);
          ++exec_queue;
        }
      }
    }

    // Run task for main thread
    static_cast<Implementation *>(this)->run_on_pack(op, pack_for_main_thread);

    for (size_t i = 0U; i < exec_queue; ++i) {
      run_queue[i].get();
    }
  }

  void run_impl(InternalOperator &op) override {
    // We use static_cast for the fast operator cast without heavy runtime
    // casts. Here we assume that current runner will always be invoked with
    // ReductionOperation object. Partially we satisfy this restriction in
    // pnm::Dispatcher.
    auto &casted_op = static_cast<ReductionOperation &>(op);
    run_operation(casted_op);
  }

  template <typename ComputeUnitImpl>
  void read_output_buffers(ReductionOperation &op,
                           ComputeUnit<ComputeUnitImpl> &compute_unit,
                           uint8_t pack, uint32_t exec_id) {
    auto dispatch_read_op = [](uint8_t output_id,
                               ComputeUnit<ComputeUnitImpl> &compute_unit,
                               pnm::views::common<uint8_t> dst) {
      switch (output_id) {
      case 0:
        compute_unit.read_buffer(BlockType::PSUM, dst);
        break;
      case 1:
        compute_unit.read_buffer(BlockType::TAGS, dst);
        break;
      default:
        throw pnm::error::make_not_sup("Output buffer number {}.", output_id);
      }
    };

    auto buffer_view = op.get_tmp_buffer(pack, exec_id);
    for (auto output_id = 0U; output_id < op.get_num_outputs(); ++output_id) {
      dispatch_read_op(output_id, compute_unit, buffer_view);
      op.write_result(output_id, pack, exec_id, buffer_view.data());
    }
  }

  void print_processed_inst(uint8_t compute_unit) {
    pnm::log::debug("----------------------------------------------------");
    uint32_t inst_num;
    device_->get_num_processed_inst(compute_unit,
                                    reinterpret_cast<uint8_t *>(&inst_num));
    pnm::log::debug("# of Processed instruction (Compute unit {}) = {}",
                    compute_unit, inst_num);
    pnm::log::debug("----------------------------------------------------");
  }

  void init_impl() override {
    std::call_once(pool_init_flag_, [this]() {
      pool_ = std::make_unique<ProducerConsumer<pnm::threads::Manager>>(
          num_of_workers_, pnm::threads::Manager{}, "ReductionRunnerThread-");
    });
  }

  sls::device::BaseDevice *device_{};
  std::unique_ptr<ProducerConsumer<pnm::threads::Manager>> pool_;
  uint64_t num_of_workers_{};
  std::once_flag pool_init_flag_;
};
} // namespace pnm::sls

#endif // PNM_REDUCTION_RUNNER_H
