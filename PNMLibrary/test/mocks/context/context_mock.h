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

#ifndef _SLS_CONTEXT_MOCK_H
#define _SLS_CONTEXT_MOCK_H

#include "core/context/sls.h"

#include "pnmlib/core/allocator.h"
#include "pnmlib/core/memory.h"

#include <linux/sls_resources.h>

#include <unistd.h>

#include <memory>

namespace test::mock {

/** @brief Context mock to support allocator customization
 */
class CustomAllocatorContext : public pnm::SlsChannelwiseContext {
public:
  CustomAllocatorContext() {
    /* This allocation need for shift memory in memory pools.
     * With this shift offsets in pools will be different.
     */
    shift_buffer_ = this->allocator()->allocate(
        getpagesize(), pnm::memory::property::AllocPolicy{SLS_ALLOC_SINGLE});
  }

  ~CustomAllocatorContext() override {
    this->allocator()->deallocate(shift_buffer_);
  }

private:
  pnm::memory::DeviceRegion shift_buffer_;
};

} // namespace test::mock
#endif
