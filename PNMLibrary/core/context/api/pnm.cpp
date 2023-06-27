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

#include "core/context/imdb.h"
#include "core/context/sls.h"

#include "common/log.h"
#include "common/make_error.h"

#include "pnmlib/core/allocator.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/transfer_manager.h"

#include "pnmlib/common/compiler.h"

#include <memory>
#include <utility>

namespace pnm {

ContextHandler PNM_API make_context(Device::Type type) {

  if constexpr (PNM_PLATFORM != HARDWARE) {
    pnm::log::info("Working in {} simulator mode.",
                   (PNM_PLATFORM == FUNCSIM) ? "FUNCTIONAL" : "PERFORMANCE");
  }

  switch (type) {
  case Device::Type::SLS_AXDIMM:
    return std::make_unique<SlsRankwiseContext>();
  case Device::Type::SLS_CXL:
    return std::make_unique<SlsChannelwiseContext>();
  case Device::Type::IMDB_CXL:
    return std::make_unique<IMDBContext>();
  }

  throw error::make_inval("Unknown context type {}.", static_cast<int>(type));
}

Context::~Context() = default;

Context::Context(Device::Type type, DevicePointer device,
                 std::unique_ptr<memory::TransferManager> transfer_manager)
    : type_{type}, device_{std::move(device)},
      transfer_manager_(std::move(transfer_manager)) {}

void Context::init_allocator(std::unique_ptr<memory::Allocator> allocator) {
  allocator_ = std::move(allocator);
}

void Context::init_runner_core(std::unique_ptr<InternalRunner> runner) {
  runner_core_ = std::move(runner);
}

} // namespace pnm
