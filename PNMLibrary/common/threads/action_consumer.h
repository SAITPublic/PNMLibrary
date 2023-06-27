/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#ifndef _ACTION_CONSUMER_H_
#define _ACTION_CONSUMER_H_

#include "common/threads/action_queue.h"

#include <memory>
#include <utility>

namespace pnm::threads {

/*! \brief Pop action from queue and execute it. */
class ActionConsumer {
public:
  explicit ActionConsumer(std::shared_ptr<ActionQueue> queue)
      : queue_{std::move(queue)} {}

  void start() {
    while (true) {
      auto func = queue_->pop();

      if (!func.valid()) {
        break;
      }

      func();
    }
  }

private:
  // The main owner is ActionQueueManager
  // Take ownership over ActionQueue if main thread destroy unordered_map
  std::shared_ptr<ActionQueue> queue_ = nullptr;
};

} // namespace pnm::threads

#endif //_ACTION_CONSUMER_H_
