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

#ifndef _SLS_SECURE_RUNNER_H
#define _SLS_SECURE_RUNNER_H

#include "pnmlib/secure/base_device.h"
#include "pnmlib/secure/base_runner.h"

#include "pnmlib/common/views.h"

#include <cstdint>

namespace pnm::sls::secure {
/*! Builder to create sls run strategy depend on strategy type
 *
 * [6/1/22] the SlsDeviceRunStrategy can be:
 *  - pnm::sls::secure::SlsSync
 *  - pnm::sls::secure::SlsProducerConsumer<ThreadCreationStrategy>
 *
 * @tparam SlsDeviceRunStrategy -- class that defines sls run strategy.
 * */
template <typename SlsDeviceRunStrategy> struct SlsOffloadingStrategyFactory {
  static SlsDeviceRunStrategy create_strategy() {
    return SlsDeviceRunStrategy{};
  }
};

/*! \brief Class to manage OperationExecutor lifetime.
 *
 * [6/1/22] the Executor can be:
 *  - pnm::sls::secure::OperationExecutor<T, SlsDevice, SlsDeviceRunStrategy>
 *
 * @tparam Executor -- instantiation of OperationExecutor or other class that
 * manage secure sls
 * */
template <typename Executor> class Runner : public IRunner {
public:
  using ExecutorType = Executor;
  Runner()
      : exec_{SlsOffloadingStrategyFactory<
            typename Executor::sls_runner_type>::create_strategy()} {}

private:
  void init_impl(const DeviceArguments *args) override {
    exec_.device_init(args);
  }

  void load_tables_impl(const void *data,
                        pnm::views::common<const uint32_t> num_entries,
                        uint32_t sparse_feature_size,
                        bool include_tag) override {
    exec_.load_tables(data, num_entries, sparse_feature_size, include_tag);
  }
  bool run_impl(uint64_t minibatch_size,
                pnm::views::common<const uint32_t> lengths,
                pnm::views::common<const uint32_t> indices,
                pnm::views::common<uint8_t> psum,
                pnm::views::common<uint8_t> checks) override {
    return exec_.run_sls(
        minibatch_size, lengths, indices,
        pnm::views::view_cast<typename Executor::value_type>(psum), checks);
  }

  Executor exec_;
};
} // namespace pnm::sls::secure

#endif //_SLS_SECURE_RUNNER_H
