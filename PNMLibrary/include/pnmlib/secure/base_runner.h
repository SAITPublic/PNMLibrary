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

#ifndef _SLS_SECURE_RUNNER_BASE_H_
#define _SLS_SECURE_RUNNER_BASE_H_

#include "pnmlib/secure/base_device.h"

#include "pnmlib/common/views.h"

#include <cstdint>

namespace sls::secure {

/*! Base interface of Secure Runners*/
class IRunner {
public:
  virtual ~IRunner() = default;

  void init(const DeviceArguments *args) { init_impl(args); }

  void load_tables(const void *data,
                   pnm::common_view<const uint32_t> num_entries,
                   uint32_t sparse_feature_size, bool include_tag) {
    load_tables_impl(data, num_entries, sparse_feature_size, include_tag);
  }

  // [TODO:] drop 'with_tag' parameter once DLRM will be ready to call this
  // method without tag
  bool run(uint64_t minibatch_size, pnm::common_view<const uint32_t> lengths,
           pnm::common_view<const uint32_t> indices,
           pnm::common_view<uint8_t> psum,
           pnm::common_view<uint8_t> checks = {},
           [[maybe_unused]] bool with_tag = false) {
    return run_impl(minibatch_size, lengths, indices, psum, checks);
  }

private:
  virtual void init_impl(const DeviceArguments *args) = 0;

  virtual void load_tables_impl(const void *data,
                                pnm::common_view<const uint32_t> num_entries,
                                uint32_t sparse_feature_size,
                                bool include_tag) = 0;

  virtual bool run_impl(uint64_t minibatch_size,
                        pnm::common_view<const uint32_t> lengths,
                        pnm::common_view<const uint32_t> indices,
                        pnm::common_view<uint8_t> psum,
                        pnm::common_view<uint8_t> checks) = 0;
};
} // namespace sls::secure

#endif //_SLS_SECURE_RUNNER_BASE_H_
