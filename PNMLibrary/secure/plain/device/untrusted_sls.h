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

#ifndef SLS_UNTRUSTED_SLS_DEVICE_H
#define SLS_UNTRUSTED_SLS_DEVICE_H

#include "secure/common/sls_io.h"

#include "common/make_error.h"
#include "common/topology_constants.h"

#include "pnmlib/secure/base_device.h"
#include "pnmlib/secure/untrusted_sls_params.h"

#include "pnmlib/sls/embedding_tables.h"
#include "pnmlib/sls/operation.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/runner.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/views.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace pnm::sls::secure {

template <typename T> class UntrustedDevice : public IDevice {
public:
  static_assert(std::is_same_v<T, uint32_t> || std::is_same_v<T, int32_t>);
  using MemoryReader = TrivialMemoryReader;
  using MemoryWriter = pnm::sls::secure::MemoryWriter;

private:
  void init_impl(const DeviceArguments *args) override {
    assert(args);
    const auto &params = args->template get<UntrustedDeviceParams>();

    context_ = pnm::make_context(pnm::Device::Type::SLS);
    runner_ = pnm::Runner(context_);

    rows_ = std::vector<uint32_t>{params.rows.begin(), params.rows.end()};
    sparse_feature_size_ = params.sparse_feature_size;
    with_tag_ = params.with_tag;

    params_ = params;

    if (with_tag_ &&
        pnm::sls::device::topo().Bus == pnm::sls::device::BusType::CXL) {
      throw pnm::error::make_inval("SLS-CXL HW does not support tags");
    }
  }

  void load_impl(const void *src, uint64_t size) override {
    if (!src) {
      assert(false && "Source data can't be null");
    }

    uint32_t sparse_ft_full_size = io_traits<MemoryWriter>::memory_size<T>(
        sparse_feature_size_, with_tag_);

    auto host_data = pnm::views::make_view(static_cast<const uint32_t *>(src),
                                           size / sizeof(uint32_t));

    tables_ = pnm::memory::EmbeddingTables::create(
        host_data, rows_, sparse_ft_full_size, context_, params_.preference);
  }

  void run_impl(uint64_t minibatch_size,
                pnm::views::common<const uint32_t> lengths,
                pnm::views::common<const uint32_t> indices,
                pnm::views::common<uint8_t> psum) override {
    try {
      pnm::operations::SlsOperation sls_op{
          static_cast<uint32_t>(sparse_feature_size_),
          pnm::views::make_const_view(rows_), tables_.get(),
          with_tag_ ? pnm::operations::SlsOperation::Type::Uint32Tagged
                    : pnm::operations::SlsOperation::Type::Uint32};

      sls_op.set_run_params(minibatch_size, lengths, indices, psum);

      runner_.run(sls_op);
    } catch (...) {
      throw pnm::error::Failure("Unable to run sls.");
    }
  }

  pnm::ContextHandler context_;
  pnm::Runner runner_;

  UntrustedDeviceParams params_;

  std::vector<uint32_t> rows_;
  size_t sparse_feature_size_{};
  bool with_tag_{};

  pnm::memory::EmbeddingTablesHandler tables_;
};

} // namespace pnm::sls::secure

#endif // SLS_UNTRUSTED_SLS_DEVICE_H
