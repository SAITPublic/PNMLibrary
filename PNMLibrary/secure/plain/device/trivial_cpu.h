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

#ifndef _TRIVIAL_CPU_H
#define _TRIVIAL_CPU_H

#include "secure/common/sls_io.h"

#include "common/cpu_sls.h"

#include "pnmlib/secure/base_device.h"

#include "pnmlib/common/views.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sls::secure {

struct TrivialCPUArgs {
  pnm::common_view<const uint32_t> rows;
  uint64_t sparse_feature_size;
  bool with_tag;
};

/*! \brief Class that perform SLS on CPU.
 *
 * This class is one of a set of runners that called from OperationExecutor.
 * Class has run method with same argument as a SLS API.
 *
 * @tparam T is a type of table's entries.
 * */
template <typename T> class TrivialCPU : public IDevice {
public:
  using MemoryReader = TrivialMemoryReader;
  using MemoryWriter = TrivialMemoryWriter;

  void run(size_t minibatch_size, pnm::common_view<const uint32_t> lengths,
           pnm::common_view<const uint32_t> indices, pnm::common_view<T> psum);

protected:
  void run_sls(size_t minibatch_size, pnm::common_view<const uint32_t> lengths,
               pnm::common_view<const uint32_t> indices,
               pnm::common_view<T> psum);

  void run_sls_with_tag(size_t minibatch_size,
                        pnm::common_view<const uint32_t> lengths,
                        pnm::common_view<const uint32_t> indices,
                        pnm::common_view<T> psum);

private:
  void init_impl(const DeviceArguments *args) override {
    const auto &typed_args = args->get<TrivialCPUArgs>();
    rows_.insert(rows_.end(), typed_args.rows.begin(), typed_args.rows.end());
    sparse_feature_size_ = typed_args.sparse_feature_size;
    with_tag_ = typed_args.with_tag;
  }

  void load_impl(const void *src, uint64_t size_bytes) override {
    data_.resize(size_bytes / sizeof(T));
    MemoryWriter writer{data_.data()};
    writer.write(static_cast<const char *>(src), size_bytes);
  }

  void run_impl(uint64_t minibatch_size,
                pnm::common_view<const uint32_t> lengths,
                pnm::common_view<const uint32_t> indices,
                pnm::common_view<uint8_t> psum) override {
    run(minibatch_size, lengths, indices, pnm::view_cast<T>(psum));
  }

  std::vector<uint32_t> rows_;
  size_t sparse_feature_size_{};
  bool with_tag_{};

  std::vector<T> data_;
};

template <typename T>
void TrivialCPU<T>::run(size_t minibatch_size,
                        pnm::common_view<const uint32_t> lengths,
                        pnm::common_view<const uint32_t> indices,
                        pnm::common_view<T> psum) {
  if (!with_tag_) {
    run_sls(minibatch_size, lengths, indices, psum);
  } else {
    run_sls_with_tag(minibatch_size, lengths, indices, psum);
  }
}

template <typename T>
void TrivialCPU<T>::run_sls(size_t minibatch_size,
                            pnm::common_view<const uint32_t> lengths,
                            pnm::common_view<const uint32_t> indices,
                            pnm::common_view<T> psum) {

  SLS<T>::compute(data_.data(), rows_, indices.data(), lengths.data(),
                  sparse_feature_size_, minibatch_size, psum.data());
}

template <typename T>
void TrivialCPU<T>::run_sls_with_tag(size_t minibatch_size,
                                     pnm::common_view<const uint32_t> lengths,
                                     pnm::common_view<const uint32_t> indices,
                                     pnm::common_view<T> psum) {
  SLS<T>::compute_with_tag(data_.data(), rows_, indices.data(), lengths.data(),
                           sparse_feature_size_, minibatch_size, psum.data());
}

} // namespace sls::secure

#endif // _TRIVIAL_CPU_H
