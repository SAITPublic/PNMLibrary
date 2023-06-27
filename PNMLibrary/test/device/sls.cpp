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

#include "sls/operation/inst_generator.h"

#include "core/device/sls/base.h"
#include "core/device/sls/constants.h"

#include "hw/axdimm_hw_config.h"

#include "pnmlib/core/device.h"

#include "pnmlib/common/misc_utils.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

TEST(SLSDevice, ProcessedInstrNumUpdate) {
  auto dev =
      pnm::sls::device::BaseDevice::make_device(pnm::Device::Type::SLS_CXL);

  // use unaligned number of instructions to count
  const size_t trace_sz = 29;

  // generate instr vector
  std::vector<uint64_t> trace(trace_sz,
                              static_cast<uint64_t>(pnm::sls::SPARSE_FT_SUM_U32)
                                  << INSTR_OPCODE_OFFSET);
  trace.front() = 0;
  trace.back() |= 1UL << INSTR_TRACE_END_OFFSET;

  const uint8_t rank = 0;
  dev->write_inst_block(rank, reinterpret_cast<const uint8_t *>(trace.data()),
                        pnm::utils::byte_size_of_container(trace));
  dev->exec_sls_register(rank)->store(dev->SLS_ENABLE_VALUE);
  ASSERT_EQ(dev->psum_poll_register(rank)->load(),
            pnm::sls::device::POLL_FINISH_VALUE);

  uint32_t inst_num;
  dev->get_num_processed_inst(rank, reinterpret_cast<uint8_t *>(&inst_num));

  EXPECT_EQ(inst_num, trace.size());
}
