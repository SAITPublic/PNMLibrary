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

#ifndef PNM_RUNNER_H
#define PNM_RUNNER_H

#include "context.h"

namespace pnm {

class Runner {
public:
  Runner() = default;

  explicit Runner(const pnm::ContextHandler &context)
      : context_{context.get()} {}

  template <typename T> void run(T &op) const {
    context_->dispatcher().invoke(op, context_);
  }

private:
  pnm::Context *context_{};
};

} // namespace pnm

#endif // PNM_RUNNER_H
