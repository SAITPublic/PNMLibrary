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

#ifndef _IMDB_CONTEXT_H_
#define _IMDB_CONTEXT_H_

#include "imdb/operation/scan.h"

#include "core/memory/imdb/transfer_manager.h"
#include "core/memory/sequential_allocator.h"
#include "core/operation_runner/imdb/runner.h"

#include "pnmlib/imdb/scan.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/dispatcher.h"

#include <memory>

namespace pnm {
class ImdbContext : public Context {
public:
  ImdbContext()
      : Context(Device::Type::IMDB, Device::make_device(Device::Type::IMDB),
                std::make_unique<pnm::memory::ImdbTransferManager>()) {
    init_allocator(
        std::make_unique<pnm::memory::SequentialAllocator>(device().get()));
    init_runner_core(std::make_unique<imdb::Runner>(device().get()));
    define_supported_operation();
  }

  void define_supported_operation() {
    const Dispatcher::internal_function<pnm::operations::Scan> scan_dispatch =
        [](pnm::operations::Scan *op, pnm::Context *ctx) {
          pnm::imdb::ScanOperation back_operator(*op);

          ctx->runner_core()->run(back_operator);

          op->set_execution_time(back_operator.execution_time());
          op->set_result_size(back_operator.result_size());
        };

    this->dispatcher().register_function(scan_dispatch);
  }
};
} // namespace pnm

#endif //_IMDB_CONTEXT_H_
