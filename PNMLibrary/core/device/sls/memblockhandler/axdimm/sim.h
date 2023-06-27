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

#ifndef _SLS_AXDIMM_SIM_MEMBLOCKHANDLER_H_
#define _SLS_AXDIMM_SIM_MEMBLOCKHANDLER_H_

#include "base.h"

#include "core/device/sls/memory_map.h"
#include "core/device/sls/simulator/axdimm.h"

#include <linux/sls_resources.h>

#include <cstddef>
#include <cstdint>

namespace pnm::sls::device {

class AxdimmSimulatorPsumReader {
public:
  explicit AxdimmSimulatorPsumReader(AxdimmSimulator &simulator)
      : simulator_(&simulator) {}

  void operator()(const memAddr &mem_addr, uint8_t compute_unit,
                  sls_mem_blocks_e block_type, uint32_t offset,
                  uint8_t *buf_out, size_t read_size);

private:
  AxdimmSimulator *simulator_;
};

class AxdimmSimulatorTagsReader {
public:
  explicit AxdimmSimulatorTagsReader(AxdimmSimulator &simulator)
      : simulator_(&simulator) {}

  void operator()(const memAddr &mem_addr, uint8_t compute_unit,
                  sls_mem_blocks_e block_type, uint32_t offset,
                  uint8_t *buf_out, size_t read_size);

private:
  AxdimmSimulator *simulator_;
};

class AxdimmSimulatorMemBlockHandler final
    : public AxdimmMemBlockHandler<AxdimmSimulatorMemBlockHandler> {
public:
  AxdimmSimulatorMemBlockHandler()
      : simulator_(mem_map()), psum_reader_(simulator_),
        tags_reader_(simulator_) {}

  // [TODO: @a.korzun] this is a temporary hack to support simulator method
  // calls from the outside of the memblockhandler. Existing code should be
  // reworked and this method should then be removed from the public API.
  AxdimmSimulator &simulator() noexcept { return simulator_; }

  auto psum_reader() const { return psum_reader_; }
  auto tags_reader() const { return tags_reader_; }

private:
  // For debug
  uint32_t get_mem_val(uint8_t compute_unit, uint32_t type, uint64_t offset);
  uint64_t get_mem_addr(uint8_t compute_unit, uint32_t type, uint64_t offset);
  void write_reg(uint8_t *reg_addr, uint32_t val);

  AxdimmSimulator simulator_;

  AxdimmSimulatorPsumReader psum_reader_;
  AxdimmSimulatorTagsReader tags_reader_;
};

} // namespace pnm::sls::device

#endif //_SLS_AXDIMM_SIM_MEMBLOCKHANDLER_H_
