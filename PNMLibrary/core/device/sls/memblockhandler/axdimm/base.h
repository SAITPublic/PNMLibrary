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

#ifndef _AXDIMM_MEMBLOCKHANDLER_H_
#define _AXDIMM_MEMBLOCKHANDLER_H_

#include "core/device/sls/memblockhandler/base.h"
#include "core/device/sls/memblockhandler/block_ops.h"
#include "core/device/sls/utils/rank_address.h"

#include <linux/sls_resources.h>

#include <cstdint>

namespace pnm::sls::device {

template <typename AxdimmMemBlockHandlerImpl>
class AxdimmMemBlockHandler
    : public SlsMemBlockHandlerImpl<AxdimmMemBlockHandlerImpl> {
public:
  auto inst_writer() const { return RankedBlockWriter{}; }

  auto cfgr_writer() const { return RankedBlockWriter{}; }
  auto cfgr_reader() const { return RankedBlockReader{}; }

  auto psum_writer() const { return UnsupportedBlockWriter{}; }
  /* tags_reader() must be implemented in derived class */

  auto tags_writer() const { return UnsupportedBlockWriter{}; }
  /* psum_reader() must be implemented in derived class */

  void *get_block_ptr_impl(sls_mem_blocks_e type, uint8_t compute_unit,
                           uint32_t offset) {
    const auto addr = this->mem_map()[compute_unit].addr[type];
    return (InterleavedPointer(addr, cunit_to_ha(compute_unit)) + offset)
        .as<void>();
  }
};

} // namespace pnm::sls::device

#endif //_AXDIMM_MEMBLOCKHANDLER_H_
