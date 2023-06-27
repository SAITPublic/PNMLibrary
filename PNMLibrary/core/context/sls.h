/*
 *  Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 *  This software is proprietary of Samsung Electronics.
 *  No part of this software, either material or conceptual may be copied or
 *  distributed, transmitted, transcribed, stored in a retrieval system or
 *  translated into any human or computer language in any form by any means,
 *  electronic, mechanical, manual or otherwise, or disclosed to third parties
 *  without the express written permission of Samsung Electronics.
 */

#ifndef PNM_SLS_CONTEXT_H
#define PNM_SLS_CONTEXT_H

#include "sls/compute_unit/axdimm_compute_unit.h"
#include "sls/compute_unit/cxl_compute_unit.h"
#include "sls/operation/internal.h"

#include "core/device/sls/base.h"
#include "core/memory/ranked_allocator.h"
#include "core/memory/sls/axdimm/transfer_manager.h"
#include "core/memory/sls/cxl/channel_transfer_manager.h"
#include "core/operation_runner/internal_runner.h"
#include "core/operation_runner/reduction/axdimm_runner.h"
#include "core/operation_runner/reduction/cxl_runner.h"

#include "common/timer.h"
#include "common/topology_constants.h"

#include "pnmlib/sls/operation.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/dispatcher.h"

#include <memory>

namespace pnm {

struct SlsChannelwiseContext : public Context {
public:
  SlsChannelwiseContext()
      : Context(Device::Type::SLS_CXL,
                Device::make_device(Device::Type::SLS_CXL),
                std::make_unique<memory::ChannelTransferManager>()) {
    init_allocator(std::make_unique<memory::RankedAllocator>(
        device().get(), pnm::device::topo().NumOfChannels));
    init_runner_core(std::make_unique<
                     pnm::sls::CXLReductionRunner<pnm::sls::CXLComputeUnit>>(
        device().get()));
    define_supported_operation();
  }

  void define_supported_operation() {
    const Dispatcher::internal_function<PNMSLSOperation> sls_dispatch =
        [](PNMSLSOperation *op, pnm::Context *ctx) {
          MEASURE_TIME();
          auto internal_op = sls::SLSOperationChanneled(
              *op, *ctx->device()->as<pnm::sls::device::BaseDevice>());
          ctx->runner_core()->run(internal_op);
        };

    this->dispatcher().register_function(sls_dispatch);
  }
};

class SlsRankwiseContext : public Context {
public:
  SlsRankwiseContext()
      : Context(Device::Type::SLS_AXDIMM,
                Device::make_device(Device::Type::SLS_AXDIMM),
                std::make_unique<memory::AXDIMMTransferManager>()) {
    init_allocator(std::make_unique<memory::RankedAllocator>(
        device().get(), pnm::device::topo().NumOfRanks));
    init_runner_core(
        std::make_unique<
            pnm::sls::AXDIMMReductionRunner<pnm::sls::AXDIMMComputeUnit>>(
            device().get()));
    define_supported_operation();
  }

  void define_supported_operation() {
    const Dispatcher::internal_function<PNMSLSOperation> sls_dispatch =
        [](PNMSLSOperation *op, pnm::Context *ctx) {
          MEASURE_TIME();
          auto internal_op = sls::SLSOperationRanked(
              *op, *ctx->device()->as<pnm::sls::device::BaseDevice>());
          ctx->runner_core()->run(internal_op);
        };

    this->dispatcher().register_function(sls_dispatch);
  }
};

} // namespace pnm

#endif // PNM_SLS_CONTEXT_H
