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

#ifndef PNM_CXL_REDUCTION_RUNNER_IMPL_H
#define PNM_CXL_REDUCTION_RUNNER_IMPL_H

#include "core.h"

#include "sls/compute_unit/compute_unit.h"
#include "sls/operation/reduction_operation.h"

#include "common/log.h"
#include "common/memory/barrier.h"

#include "pnmlib/core/device.h"

#include "pnmlib/common/views.h"

#include <linux/sls_resources.h>

#include <sys/ioctl.h>

#include <algorithm>
#include <cstdint>

namespace pnm::sls {

template <typename ComputeUnit>
class CxlReductionRunner
    : public ReductionRunner<CxlReductionRunner<ComputeUnit>> {
  using ReductionRunner<CxlReductionRunner<ComputeUnit>>::device_;
  using ReductionRunner<CxlReductionRunner<ComputeUnit>>::read_output_buffers;

public:
  explicit CxlReductionRunner(Device *sls_device, unsigned int num_of_workers)
      : ReductionRunner<CxlReductionRunner<ComputeUnit>>(sls_device,
                                                         num_of_workers) {}

  void run_on_pack(ReductionOperation &op, uint8_t pack);

private:
  void process_execs(ReductionOperation &op, uint8_t pack,
                     ComputeUnit &compute_unit);

  void reset_output_buffers(uint8_t cunit_id, uint64_t size) const;

  void nop() const;
};

template <typename ComputeUnit>
void CxlReductionRunner<ComputeUnit>::run_on_pack(ReductionOperation &op,
                                                  uint8_t pack) {
  ComputeUnit compute_unit(device_, pack);
  CxlReductionRunner<ComputeUnit>::process_execs(op, pack, compute_unit);
}

template <typename ComputeUnit>
void CxlReductionRunner<ComputeUnit>::process_execs(ReductionOperation &op,
                                                    uint8_t pack,
                                                    ComputeUnit &compute_unit) {
  const auto num_execs = op.get_num_exec_iterations(pack);

  for (auto exec_id = 0U; exec_id < num_execs; ++exec_id) {
    const auto &trace =
        op.generate_instructions(pack, compute_unit.id(), exec_id);

    compute_unit.write_buffer(
        BlockType::INST,
        pnm::views::view_cast<const uint8_t>(pnm::views::make_view(trace)));

    reset_output_buffers(compute_unit.id(),
                         op.get_tmp_buffer(pack, exec_id).size());

    compute_unit.run();

    read_output_buffers(op, compute_unit, pack, exec_id);

    if constexpr (PNM_PLATFORM == HARDWARE) {
      nop();
    }
  }
}

template <typename ComputeUnit>
void CxlReductionRunner<ComputeUnit>::reset_output_buffers(
    uint8_t cunit_id, uint64_t size) const {
  auto *buffer = device_->mem_block_handler()->get_mem_block_ptr(SLS_BLOCK_PSUM,
                                                                 cunit_id, 0);
  std::fill_n(static_cast<uint8_t *>(buffer), size, 0x0);
  if constexpr (PNM_PLATFORM != FUNCSIM) {
    pnm::memory::flush(buffer, size);
    pnm::memory::mfence();
  }
}

template <typename ComputeUnit>
void CxlReductionRunner<ComputeUnit>::nop() const {
  // For some reason, the device will not hang after multiple execs if we
  // make any syscall. So, we just make a NOP ioctl to our resource device.
  // [TODO: @y-lavrinenko] Get rid when we have stable HW
  auto ret = ioctl(device_->get_resource_fd(), NOP);
  if (ret != 0) {
    PNM_LOG_ERROR("Fail to execute NOP ioctl");
  }
}

} // namespace pnm::sls

#endif // PNM_CXL_REDUCTION_RUNNER_IMPL_H
