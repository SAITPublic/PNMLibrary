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
#include "core/device/sls/compute_device.h"
#include "core/device/sls/simulator/axdimm_compute_device.h"
#include "core/device/sls/simulator/cxl_compute_device.h"
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
#include <type_traits>

namespace pnm {
using pnm::sls::device::topo;

struct SlsChannelwiseContext : public Context {
public:
  SlsChannelwiseContext()
      : Context(Device::Type::SLS, Device::make_device(Device::Type::SLS),
                std::make_unique<memory::ChannelTransferManager>()) {
    init_allocator(std::make_unique<memory::RankedAllocator>(
        device().get(), topo().NumOfCUnits));

    using compute_unit_type = std::conditional_t<
        PNM_PLATFORM != HARDWARE,
        pnm::sls::CxlComputeUnit<pnm::sls::device::CxlSimulatorComputeDevice>,
        pnm::sls::CxlComputeUnit<pnm::sls::device::ComputeDevice>>;

    init_runner_core(
        std::make_unique<pnm::sls::CxlReductionRunner<compute_unit_type>>(
            device().get(), topo().NumOfCUnits));
    define_supported_operation();
  }

  void define_supported_operation() {
    const Dispatcher::internal_function<pnm::operations::SlsOperation>
        sls_dispatch =
            [](pnm::operations::SlsOperation *op, pnm::Context *ctx) {
              MEASURE_TIME();
              auto internal_op = sls::SparseReductionChanneled(
                  *op, *ctx->device()->as<pnm::sls::device::BaseDevice>());
              ctx->runner_core()->run(internal_op);
            };

    this->dispatcher().register_function(sls_dispatch);
  }
};

class SlsRankwiseContext : public Context {
public:
  SlsRankwiseContext()
      : Context(Device::Type::SLS, Device::make_device(Device::Type::SLS),
                std::make_unique<memory::AxdimmTransferManager>()) {
    init_allocator(std::make_unique<memory::RankedAllocator>(
        device().get(), topo().NumOfCUnits));

    using compute_unit_type = std::conditional_t<
        PNM_PLATFORM != HARDWARE,
        pnm::sls::AxdimmComputeUnit<
            pnm::sls::device::AxdimmSimulatorComputeDevice>,
        pnm::sls::AxdimmComputeUnit<pnm::sls::device::ComputeDevice>>;

    init_runner_core(
        std::make_unique<pnm::sls::AxdimmReductionRunner<compute_unit_type>>(
            device().get(), topo().NumOfCUnits));
    define_supported_operation();
  }

  void define_supported_operation() {
    const Dispatcher::internal_function<pnm::operations::SlsOperation>
        sls_dispatch =
            [](pnm::operations::SlsOperation *op, pnm::Context *ctx) {
              MEASURE_TIME();
              auto internal_op = sls::SparseReductionRanked(
                  *op, *ctx->device()->as<pnm::sls::device::BaseDevice>());
              ctx->runner_core()->run(internal_op);
            };

    this->dispatcher().register_function(sls_dispatch);
  }
};

} // namespace pnm

#endif // PNM_SLS_CONTEXT_H
