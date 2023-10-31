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

#ifndef PNM_INTERNAL_RUNNER_H
#define PNM_INTERNAL_RUNNER_H

#include "core/operation/internal.h"

namespace pnm {
class InternalRunner {
public:
  void run(InternalOperator &op) { run_impl(op); }
  void init() { init_impl(); }

  virtual ~InternalRunner() = default;

private:
  virtual void run_impl(pnm::InternalOperator &op) = 0;
  virtual void init_impl() = 0;
};

} // namespace pnm

#endif // PNM_INTERNAL_RUNNER_H
