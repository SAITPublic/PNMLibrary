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

#include "base.h"

#include "core/device/sls/memblockhandler/axdimm/hardware.h"
#include "core/device/sls/memblockhandler/axdimm/sim.h"
#include "core/device/sls/memblockhandler/cxl/hardware.h"
#include "core/device/sls/memblockhandler/cxl/sim.h"
#include "core/device/sls/utils/memory_map.h"

#include "common/make_error.h"
#include "common/topology_constants.h" // NOLINT(misc-include-cleaner)

#include "pnmlib/sls/control.h"

#include <linux/sls_resources.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

namespace pnm::sls::device {

void ISlsMemBlockHandler::init(const MemInfo &info, int device_fd) {
  mem_map_.init_device_memory(info, device_fd);
  mem_map_.print();
}

void ISlsMemBlockHandler::write_block(sls_mem_blocks_e type,
                                      uint8_t compute_unit, uint32_t offset,
                                      const uint8_t *buf_in,
                                      size_t write_size) {
  assert(compute_unit < topo().NumOfCUnits);
  const auto &mem_addr = mem_map_[compute_unit];
  write_block_impl(mem_addr, compute_unit, type, offset, buf_in, write_size);
}

void ISlsMemBlockHandler::read_block(sls_mem_blocks_e type,
                                     uint8_t compute_unit, uint32_t offset,
                                     uint8_t *buf_out, size_t read_size) {
  assert(compute_unit < topo().NumOfCUnits);
  const auto &mem_addr = mem_map_[compute_unit];
  read_block_impl(mem_addr, compute_unit, type, offset, buf_out, read_size);
}

void *ISlsMemBlockHandler::get_mem_block_ptr(sls_mem_blocks_e type,
                                             uint8_t compute_unit,
                                             uint32_t offset) {
  return get_block_ptr(type, compute_unit, offset);
}

size_t
ISlsMemBlockHandler::get_min_block_size(sls_mem_blocks_e block_type) const {
  return mem_map_.get_min_block_size(block_type);
}

size_t ISlsMemBlockHandler::get_base_memory_size() const {
  return mem_map_.get_all_cunits_block_size(SLS_BLOCK_BASE);
}

std::unique_ptr<ISlsMemBlockHandler>
ISlsMemBlockHandler::create(BusType bus_type) {
  switch (bus_type) {
  case BusType::AXDIMM: {
    using ConcreteMemBlockHandlerType =
        std::conditional_t<PNM_PLATFORM != HARDWARE,
                           AxdimmSimulatorMemBlockHandler,
                           AxdimmHardwareMemBlockHandler>;
    return std::make_unique<ConcreteMemBlockHandlerType>();
  }
  case BusType::CXL: {
    using ConcreteMemBlockHandlerType =
        std::conditional_t<PNM_PLATFORM != HARDWARE,
                           CxlSimulatorMemBlockHandler,
                           CxlHardwareMemBlockHandler>;
    return std::make_unique<ConcreteMemBlockHandlerType>();
  }
  }
  throw pnm::error::make_inval("Device type.");
}

} // namespace pnm::sls::device
