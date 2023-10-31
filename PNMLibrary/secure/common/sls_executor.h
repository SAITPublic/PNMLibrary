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

#ifndef SLS_SECURE_EXECUTOR_H
#define SLS_SECURE_EXECUTOR_H

#include "secure/encryption/sls_dlrm_preprocessor.h"
#include "secure/encryption/sls_postrocessor.h"

#include "pnmlib/common/128bit_math.h"
#include "pnmlib/common/rowwise_view.h"
#include "pnmlib/common/variable_row_view.h"
#include "pnmlib/common/views.h"

#include <cstddef>
#include <cstdint>
#include <future>
#include <iterator>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

namespace pnm::sls::secure {

/*! \brief Class to perform accelerated secure SLS
 *
 * This class performs SLS over embedding table. Class can work in SGX enclave.
 * The SlsOperationStrategy is used to interact with SLS (or other) device.
 *
 * @tparam T -- is a type of table entry
 * @tparam SlsDevice -- is a strategy for SLS operation (untrusted).
 * @tparam SlsExecutionStrategy -- is a strategy that define SLS execution
 * process on device
 * */
template <typename T, typename SlsDevice, typename SlsExecutionStrategy>
class OperationExecutor {
public:
  using value_type = T;
  using sls_device_type = SlsDevice;
  using sls_runner_type = SlsExecutionStrategy;

  explicit OperationExecutor(SlsExecutionStrategy runner = {})
      : exec_{std::move(runner)}, buffers_{} {}

  void load_tables(const void *src,
                   pnm::views::common<const uint32_t> num_entries,
                   uint32_t sparse_feature_size, bool with_tag = false) {
    DLRMPreprocessor<T, typename SlsDevice::MemoryReader,
                     typename SlsDevice::MemoryWriter>
        preprocessor(
            src, std::vector<uint32_t>(num_entries.begin(), num_entries.end()),
            sparse_feature_size);

    const auto buffer_size = preprocessor.encrypted_buffer_size(with_tag);
    std::vector<uint8_t> buffer(buffer_size);
    postprocessor_ = preprocessor.load(buffer.data(), with_tag);

    device_.load(buffer.data(), buffer_size);

    num_tables_ = num_entries.size();
    sparse_feature_size_ = sparse_feature_size;
    with_tag_ = with_tag;
  }

  template <typename... TArgs> void device_init(TArgs &&...args) {
    device_.init(std::forward<TArgs>(args)...);
  }

  template <typename... TArgs> void set_device_args(TArgs &&...args) {
    device_.set_extra_run_args(std::forward<TArgs>(args)...);
  }

  bool run_sls(uint64_t minibatch_size,
               pnm::views::common<const uint32_t> lengths,
               pnm::views::common<const uint32_t> indices,
               pnm::views::common<T> psum,
               pnm::views::common<uint8_t> checks = {});

private:
  void reorder_indices(uint64_t minibatch_size,
                       pnm::views::common<const uint32_t> lengths,
                       pnm::views::common<const uint32_t> indices);

  void execute_sls(uint64_t minibatch_size,
                   pnm::views::common<const uint32_t> lengths,
                   pnm::views::common<const uint32_t> indices,
                   pnm::views::common<T> augmented_psum_view);

  bool decrypt(uint64_t minibatch_size, pnm::views::common<T> psum_buffer,
               pnm::views::common<uint8_t> check);

  void make_user_psum(pnm::views::common<T> user_psum,
                      pnm::views::common<T> augmented_psum,
                      pnm::views::common<uint8_t> check);

  static constexpr auto TAG_SIZE = sizeof(pnm::types::uint128_t) / sizeof(T);

  size_t num_tables_{};
  uint64_t sparse_feature_size_{};
  bool with_tag_{};
  std::unique_ptr<IPostprocessor<T>> postprocessor_;

  SlsDevice device_;
  SlsExecutionStrategy exec_;

  struct {
    std::vector<T> cpsum; // psum from device, only in tag mode
    std::vector<T> epsum; // psum from encryption engine
    std::vector<requests_view> indices;
    std::vector<std::future<void>> results;
  } buffers_; // Buffers should be defined after device
};

template <typename T, typename SlsOperationStrategy,
          typename SlsDeviceRunStrategy>
bool OperationExecutor<T, SlsOperationStrategy, SlsDeviceRunStrategy>::run_sls(
    uint64_t minibatch_size, pnm::views::common<const uint32_t> lengths,
    pnm::views::common<const uint32_t> indices, pnm::views::common<T> psum,
    pnm::views::common<uint8_t> checks) {

  pnm::views::common<T> augmented_psum_view = psum;
  if (with_tag_) { // We need extra storage only in tagged case
    buffers_.cpsum.resize(minibatch_size * num_tables_ *
                          (sparse_feature_size_ + TAG_SIZE));
    augmented_psum_view = pnm::views::make_view(buffers_.cpsum);
  }

  // Reallocate temporary buffer for EPSUM
  buffers_.epsum.resize(augmented_psum_view.size());

  // Prepare indices for SLS over OTPs
  reorder_indices(minibatch_size, lengths, indices);

  // Start SLS on device and OTPs
  execute_sls(minibatch_size, lengths, indices, augmented_psum_view);

  if (with_tag_) {
    auto verification_status =
        decrypt(minibatch_size, augmented_psum_view, checks);

    // Copy psum from internal buffer to user memory
    make_user_psum(psum, augmented_psum_view, checks);
    return verification_status;
  }
  return decrypt(minibatch_size, augmented_psum_view,
                 pnm::views::common<uint8_t>());
}

template <typename T, typename SlsOperationStrategy,
          typename SlsDeviceRunStrategy>
void OperationExecutor<T, SlsOperationStrategy, SlsDeviceRunStrategy>::
    reorder_indices(uint64_t minibatch_size,
                    pnm::views::common<const uint32_t> lengths,
                    pnm::views::common<const uint32_t> indices) {
  buffers_.indices.clear();
  buffers_.indices.reserve(num_tables_);

  const auto *idx_it = indices.begin();

  auto lengths_v = pnm::views::make_rowwise_view(lengths.begin(), lengths.end(),
                                                 minibatch_size);
  auto lengths_it = lengths_v.begin();

  for (auto tid = 0UL; tid < num_tables_; ++tid) {
    auto lookups_in_minibatch =
        std::accumulate(lengths_it->begin(), lengths_it->end(), 0ULL);
    buffers_.indices.emplace_back(
        tid, pnm::views::make_variable_row_view(
                 idx_it, idx_it + lookups_in_minibatch, *lengths_it));
    std::advance(idx_it, lookups_in_minibatch);
    ++lengths_it;
  }
}

template <typename T, typename SlsOperationStrategy,
          typename SlsDeviceRunStrategy>
bool OperationExecutor<T, SlsOperationStrategy, SlsDeviceRunStrategy>::decrypt(
    uint64_t minibatch_size, pnm::views::common<T> psum_buffer,
    pnm::views::common<uint8_t> check) {

  const auto psum_size = minibatch_size * num_tables_ * sparse_feature_size_;
  auto psum_rv = pnm::views::make_rowwise_view(psum_buffer.begin(),
                                               psum_buffer.begin() + psum_size,
                                               sparse_feature_size_);

  const auto augmented_row_size = sparse_feature_size_ + TAG_SIZE * with_tag_;
  auto epsum_v = pnm::views::make_rowwise_view(
      buffers_.epsum.data(), buffers_.epsum.data() + buffers_.epsum.size(),
      augmented_row_size, 0, TAG_SIZE * with_tag_);

  if (with_tag_) {
    auto ctag_v = pnm::views::make_rowwise_view(psum_buffer.begin() + psum_size,
                                                psum_buffer.end(), TAG_SIZE);

    auto tag_v = pnm::views::make_rowwise_view(
        buffers_.epsum.data(), buffers_.epsum.data() + buffers_.epsum.size(),
        augmented_row_size, sparse_feature_size_, 0);

    return postprocessor_->decrypt_psum(psum_rv, epsum_v, ctag_v, tag_v, check);
  }

  return postprocessor_->decrypt_psum(psum_rv, epsum_v);
}

template <typename T, typename SlsDevice, typename SlsDeviceRunStrategy>
void OperationExecutor<T, SlsDevice, SlsDeviceRunStrategy>::make_user_psum(
    pnm::views::common<T> user_psum, pnm::views::common<T> augmented_psum,
    pnm::views::common<uint8_t> check) {
  const auto psum_size = user_psum.size();
  auto aug_psum_rows = pnm::views::make_rowwise_view(
      augmented_psum.begin(), augmented_psum.begin() + psum_size,
      sparse_feature_size_);
  auto user_psum_rows = pnm::views::make_rowwise_view(
      user_psum.begin(), user_psum.end(), sparse_feature_size_);

  auto *is_row_ok = check.begin();
  auto copy_func = [&is_row_ok](auto aug_row, auto user_row) {
    if (*(is_row_ok++)) {
      std::copy(aug_row.begin(), aug_row.end(), user_row.begin());
    } else {
      // [TODO: @y-lavrinenko] Throw an exception when device will be stable
    }
    return aug_row;
  };

  std::transform(aug_psum_rows.begin(), aug_psum_rows.end(),
                 user_psum_rows.begin(), aug_psum_rows.begin(), copy_func);
}

template <typename T, typename SlsDevice, typename SlsExecutionStrategy>
void OperationExecutor<T, SlsDevice, SlsExecutionStrategy>::execute_sls(
    uint64_t minibatch_size, pnm::views::common<const uint32_t> lengths,
    pnm::views::common<const uint32_t> indices,
    pnm::views::common<T> augmented_psum_view) {
  // SLS on OTPs/OTPs and TAGs, each table can be processed in parallel
  auto augmented_row_size = sparse_feature_size_ + TAG_SIZE * with_tag_;

  auto *epsum_buffer_it = buffers_.epsum.data();

  auto postproc_task = [this](auto &&...args) {
    postprocessor_->perform_sls(std::forward<decltype(args)>(args)...);
  };

  buffers_.results.reserve(buffers_.indices.size());
  for (const auto &request_it : buffers_.indices) {
    auto table_indices = pnm::views::make_view(&request_it, &request_it + 1);

    const auto epsum_buffer_per_table =
        augmented_row_size * request_it.second.size();
    auto epsum_v = pnm::views::make_rowwise_view(
        epsum_buffer_it, epsum_buffer_it + epsum_buffer_per_table,
        augmented_row_size, 0, TAG_SIZE * with_tag_);

    if (with_tag_) {
      auto tag_v = pnm::views::make_rowwise_view(
          epsum_buffer_it, epsum_buffer_it + epsum_buffer_per_table,
          augmented_row_size, sparse_feature_size_, 0);

      buffers_.results.emplace_back(
          exec_.run(postproc_task, table_indices, epsum_v, tag_v));
    } else {
      buffers_.results.emplace_back(
          exec_.run(postproc_task, table_indices, epsum_v));
    }

    std::advance(epsum_buffer_it, epsum_buffer_per_table);
  }

  // Run SLS on device. Do not offload this work to another thread because it
  // hurts peformance.
  device_(minibatch_size, lengths, indices,
          pnm::views::view_cast<uint8_t>(augmented_psum_view));

  // Wait for SLS on OTPs
  for (auto &result : buffers_.results) {
    result.get();
  }
  buffers_.results.clear();
}

} // namespace pnm::sls::secure

#endif // SLS_SECURE_EXECUTOR_H
