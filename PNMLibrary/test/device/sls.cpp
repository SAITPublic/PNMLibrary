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

#include "core/device/sls/base.h"
#include "core/device/sls/simulator/sim.h"
#include "core/device/sls/utils/constants.h"
#include "core/device/sls/utils/inst_generator.h"

#include "hw/pnm_hw_instr_constants.h"

#include "pnmlib/common/misc_utils.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>

TEST(SlsDevice, ProcessedInstrNumUpdate) {
  auto dev = pnm::sls::device::BaseDevice::make_device();

  // use unaligned number of instructions to count
  const size_t trace_sz = 29;

  // generate instr vector
  std::vector<uint64_t> trace(
      trace_sz, static_cast<uint64_t>(pnm::sls::Opcode::SPARSE_FT_SUM_U32)
                    << INSTR_OPCODE_OFFSET);
  trace.front() = 0;
  trace.back() |= 1UL << INSTR_TRACE_END_OFFSET;

  const uint8_t rank = 0;
  auto *atomic_poll_reg =
      pnm::utils::as_vatomic<uint32_t>(dev->psum_poll_register(rank));
  atomic_poll_reg->store(0);
  // static_cast will be correct if we run the test in simulator mode
  static_cast<pnm::sls::device::SimulatorDevice *>(dev.get())
      ->get_sim()
      .enable_thread(rank);
  dev->write_inst_block(rank, reinterpret_cast<const uint8_t *>(trace.data()),
                        pnm::utils::byte_size_of_container(trace));
  pnm::utils::as_vatomic<uint32_t>(dev->exec_sls_register(rank))
      ->store(pnm::sls::device::SLS_ENABLE_VALUE);
  while (atomic_poll_reg->load() != pnm::sls::device::POLL_FINISH_VALUE) {
    std::this_thread::yield();
  }

  uint32_t inst_num;
  dev->get_num_processed_inst(rank, reinterpret_cast<uint8_t *>(&inst_num));

  EXPECT_EQ(inst_num, trace.size());
}
