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

#include "pnmlib/core/runner.h"

#include "core/operation_runner/internal_runner.h"

#include "pnmlib/core/context.h"

namespace pnm {

Runner::Runner(const pnm::ContextHandler &context) : context_{context.get()} {
  context_->runner_core()->init();
}

} // namespace pnm
