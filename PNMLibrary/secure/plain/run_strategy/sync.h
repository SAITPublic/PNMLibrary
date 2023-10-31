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

#ifndef _SYNC_RUNNER_H_
#define _SYNC_RUNNER_H_

#include <exception>
#include <functional>
#include <future>
#include <utility>

namespace pnm::sls::secure {
/*! \brief Class that makes a sequential run of workload on dev */
class SlsSync {
public:
  template <typename Func, typename... Args>
  std::future<void> run(Func &&func, Args &&...args) {
    std::promise<void> promise;
    auto future = promise.get_future();

    try {
      std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
      promise.set_value();
    } catch (...) {
      promise.set_exception(std::current_exception());
    }

    return future;
  }
};

} // namespace pnm::sls::secure

#endif // _SYNC_RUNNER_H_
