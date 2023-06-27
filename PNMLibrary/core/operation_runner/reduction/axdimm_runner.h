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

#ifndef PNM_AXDIMM_REDUCTION_RUNNER_IMPL_H
#define PNM_AXDIMM_REDUCTION_RUNNER_IMPL_H

#include "core.h"

#include "sls/compute_unit/compute_unit.h"
#include "sls/operation/inst_generator.h"
#include "sls/operation/reduction_operation.h"

#include "core/device/sls/base.h"

#include "pnmlib/core/device.h"

#include "pnmlib/common/definitions.h"
#include "pnmlib/common/views.h"

#include <cstdint>
#include <vector>

namespace pnm::sls {

template <typename ComputeUnit>
class AXDIMMReductionRunner
    : public ReductionRunner<AXDIMMReductionRunner<ComputeUnit>> {
  using ReductionRunner<AXDIMMReductionRunner<ComputeUnit>>::device_;
  using ReductionRunner<
      AXDIMMReductionRunner<ComputeUnit>>::read_output_buffers;
  using ReductionRunner<
      AXDIMMReductionRunner<ComputeUnit>>::print_processed_inst;

public:
  explicit AXDIMMReductionRunner(Device *sls_device)
      : ReductionRunner<AXDIMMReductionRunner<ComputeUnit>>(sls_device) {}

  void run_on_pack(ReductionOperation &op, uint8_t pack);

private:
  void submit_instructions(ComputeUnit &compute_unit, uint32_t exec_id,
                           const std::vector<uint64_t> &trace);

  void wait_and_start(ComputeUnit &compute_unit,
                      const std::vector<uint64_t> &trace);
};

template <typename ComputeUnit>
void AXDIMMReductionRunner<ComputeUnit>::run_on_pack(ReductionOperation &op,
                                                     uint8_t pack) {
  const uint32_t num_execs = op.get_num_exec_iterations(pack);
  ComputeUnit compute_unit(device_, pack);

  for (uint32_t i = 0; i < num_execs; ++i) {
    /* Instruction Generation */
    const auto &traces = op.generate_instructions(pack, compute_unit.id(), i);
    /* Write Instruction */
    submit_instructions(compute_unit, i, traces);
    if (i > 0) {
      read_output_buffers(op, compute_unit, pack, i - 1);
    }
  }

  /* Execute dummy instruction. According to HW guideline, one can reliably read
   * previous PSUM result _only_ after next SLS exec was triggered. This is even
   * more important in case of PSUM double buffering: HW switches "read" path
   * between buffers on each SLS trigger. All that said, to read the very last
   * execution trace result we run dummy instruction just to make HW switch PSUM
   * buffer and provide correct read pointer for last read_and_reset_psum call.
   */
  wait_and_start(compute_unit, sls::dummy_inst);
  /* Read psum output of the last non-dummy execution trace */
  read_output_buffers(op, compute_unit, pack, num_execs - 1);

  /* Wait dummy instruction to complete */
  compute_unit.wait();

  if constexpr (DEBUG) {
    print_processed_inst(compute_unit.id());
  }
}

template <typename ComputeUnit>
void AXDIMMReductionRunner<ComputeUnit>::submit_instructions(
    ComputeUnit &compute_unit, uint32_t exec_id,
    const std::vector<uint64_t> &trace) {
  /* 1st execution exception */
  if (exec_id == 0) {
    /* Write instruction & execution */
    compute_unit.write_buffer(BlockType::INST,
                              view_cast<const uint8_t>(make_view(trace)));
    compute_unit.run_async();
  } else {
    wait_and_start(compute_unit, trace);
  }
}

template <typename ComputeUnit>
void AXDIMMReductionRunner<ComputeUnit>::wait_and_start(
    ComputeUnit &compute_unit, const std::vector<uint64_t> &trace) {
  if (SINGLE_DOUBLE) {
    compute_unit.wait();
    compute_unit.write_buffer(BlockType::INST,
                              view_cast<const uint8_t>(make_view(trace)));
  } else {
    compute_unit.write_buffer(BlockType::INST,
                              view_cast<const uint8_t>(make_view(trace)));
    compute_unit.wait();
  }

  compute_unit.run_async();
}

} // namespace pnm::sls

#endif // PNM_AXDIMM_REDUCTION_RUNNER_IMPL_H
