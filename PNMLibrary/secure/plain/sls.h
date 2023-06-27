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

#ifndef SLS_SECURE_H
#define SLS_SECURE_H

#include "secure/common/sls_executor.h"
#include "secure/common/sls_runner.h"
#include "secure/plain/device/trivial_cpu.h"
#include "secure/plain/device/untrusted_sls.h"
#include "secure/plain/run_strategy/producer_consumer.h"
#include "secure/plain/run_strategy/sync.h"

#include "common/make_error.h"
#include "common/threads/workers_storage.h"

#include <charconv>
#include <cstdlib>
#include <string_view>
#include <system_error>

namespace sls::secure {

/*! Partial specialization of SLSRunStrategyFactory<SLSDeviceRunStrategy> for
 * SLSProducerConsumer<ThreadCreationStrategy> strategy
 * */
template <typename ThreadCreationStrategy>
struct SLSOffloadingStrategyFactory<
    SLSProducerConsumer<ThreadCreationStrategy>> {
  static SLSProducerConsumer<ThreadCreationStrategy> create_strategy() {
    return SLSProducerConsumer<ThreadCreationStrategy>(get_thread_count(), {},
                                                       "SecNDPThread-");
  }

  static unsigned get_thread_count() {
    /* Default SLS on OTPs threads */
    static constexpr unsigned DEFAULT_THREAD_COUNT = 8;
    static constexpr auto ENV_NAME = "SLS_OTP_NUM_THREADS";

    if (const char *thread_count_env = std::getenv(ENV_NAME)) {
      const std::string_view env_string(thread_count_env);
      unsigned env_value;

      auto [ptr, errc] =
          std::from_chars(env_string.begin(), env_string.end(), env_value);

      if (errc != std::errc()) {
        throw pnm::error::make_inval(
            "Unable to parse {} environment variable as an unsigned "
            "integer. Error = {}. Value of variable = {}.",
            ENV_NAME, std::make_error_code(errc).message(), env_string);
      }

      if (ptr != env_string.end()) {
        throw pnm::error::make_inval(
            "Unable to parse {} environment variable as an unsigned "
            "integer. It has garbage at the end. Value of variable = {}.",
            ENV_NAME, env_string);
      }

      return env_value;
    }

    return DEFAULT_THREAD_COUNT;
  }
};

template <typename T>
using SyncCPURunner = Runner<
    OperationExecutor<T, sls::secure::TrivialCPU<T>, sls::secure::SLSSync>>;

template <typename T>
using SyncSLSRunner =
    Runner<OperationExecutor<T, sls::secure::UntrustedDevice<T>,
                             sls::secure::SLSSync>>;

template <typename T>
using ProdConsSLSRunner = Runner<
    OperationExecutor<T, sls::secure::UntrustedDevice<T>,
                      sls::secure::SLSProducerConsumer<pnm::threads::Manager>>>;

template <typename T>
using ProdConsCPURunner = Runner<
    OperationExecutor<T, sls::secure::TrivialCPU<T>,
                      sls::secure::SLSProducerConsumer<pnm::threads::Manager>>>;
} // namespace sls::secure

#endif // SLS_SECURE_H
