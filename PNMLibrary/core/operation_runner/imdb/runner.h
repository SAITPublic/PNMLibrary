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

#ifndef PNM_IMDB_RUNNER_H
#define PNM_IMDB_RUNNER_H

#include "imdb/operation/scan.h"

#include "core/device/imdb/base.h"
#include "core/operation/internal.h"
#include "core/operation_runner/internal_runner.h"

#include "pnmlib/imdb/libimdb.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"

namespace pnm::imdb {

class Runner : public InternalRunner {
public:
  explicit Runner(Device *device);

private:
  void run_operator(ScanOperation &op);

  void run_and_wait(volatile ThreadCSR *CSR);

  void run_impl(pnm::InternalOperator &op) override;

  device::BaseDevice *device_;
};

} // namespace pnm::imdb

#endif // PNM_IMDB_RUNNER_H
