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
#include "pnmlib/core/accessor_builder.h"

#include "core/memory/imdb/accessor.h"
#include "core/memory/sls/accessor.h"
#include "core/memory/sls/cxl/channel_accessor.h"

#include "common/make_error.h"

#include "pnmlib/core/accessor.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/compiler.h"

#include <cstdint>
#include <memory>
#include <variant>

namespace {
template <typename... T> struct VisitorVariants : T... {
  using T::operator()...;
};
template <typename... T> VisitorVariants(T...) -> VisitorVariants<T...>;
} // namespace

PNM_API std::unique_ptr<pnm::memory::AccessorCore>
pnm::memory::create_accessor_core(const pnm::memory::DeviceRegion &region,
                                  [[maybe_unused]] Context *ctx) {
  auto on_sequential_region = [ctx](const pnm::memory::SequentialRegion &r)
      -> std::unique_ptr<pnm::memory::AccessorCore> {
    return std::make_unique<pnm::memory::IMDBAccessorCore>(r, ctx->device());
  };

  auto on_ndregion = [ctx]([[maybe_unused]] const pnm::memory::RankedRegion &r)
      -> std::unique_ptr<pnm::memory::AccessorCore> {
    if (ctx->type() == pnm::Device::Type::SLS_AXDIMM) {
      return std::make_unique<pnm::memory::SLSAccessorCore>(r, ctx->device());
    }
    if (ctx->type() == pnm::Device::Type::SLS_CXL) {
      return std::make_unique<pnm::memory::ChannelAccessorCore>(r,
                                                                ctx->device());
    }
    throw pnm::error::make_inval("Unknown context type ({}) for ndregion.",
                                 static_cast<uint64_t>(ctx->type()));
  };

  return std::visit(VisitorVariants{on_sequential_region, on_ndregion}, region);
}
