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

#ifndef SLS_DLRM_PREPROCESSOR_H
#define SLS_DLRM_PREPROCESSOR_H

#include "sls_dlrm_postprocessor.h"
#include "sls_preprocessor.h"

#include "secure/encryption/io_traits.h"
#include "secure/encryption/sls_engine.h"
#include "secure/encryption/sls_mac.h"
#include "secure/encryption/sls_postrocessor.h"

#include "pnmlib/common/128bit_math.h"
#include "pnmlib/common/misc_utils.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

namespace pnm::sls::secure {

/*! \brief Tables preprocessor for DLRM embedding tables.
 * @tparam T is an embedding entry type
 * @tparam Reader is a class for reading embedding tables
 * @tparam Writer is a class for offloading encrypted tables
 * */
template <typename T, typename Reader, typename Writer>
class DLRMPreprocessor : public IPreprocessor<T> {
public:
  template <typename TableArgT>
  DLRMPreprocessor(TableArgT table, std::vector<uint32_t> entries,
                   uint32_t sparse_feature_size)
      : tables_count_{entries.size()},
        sparse_feature_size_{sparse_feature_size},
        table_entries_(std::move(entries)),
        version_(create_table_version(table)), reader_{std::move(table)} {}

private:
  static constexpr auto BUFFER_SIZE = 8192;

  uint64_t encrypted_buffer_size_impl(bool include_tag) const override {
    auto single_row_size = io_traits<Writer>::template memory_size<T>(
        sparse_feature_size_, include_tag);
    return single_row_size *
           std::accumulate(table_entries_.begin(), table_entries_.end(), 0ULL);
  }

  /*! \brief Load table to AxDIMM memory specified by stream (output_iterator in
   * future) out. The result is unique_ptr to class with information for
   * decryption
   * */
  std::unique_ptr<IPostprocessor<T>> load_impl(void *out,
                                               bool include_tag) override;

  template <typename InT>
  uint32_t create_table_version(const InT &value) const {
    size_t hash = std::hash<InT>{}(value);

    if constexpr (sizeof(size_t) == 8) {
      const uint32_t hi = hash >> 32;
      const uint32_t low = hash & UINT32_MAX;

      return low ^ hi;
    }

    return hash;
  }

  using EEngine_t = pnm::sls::secure::EncryptionEngine<T>;
  using VEngine_t = pnm::sls::secure::VerificationEngine<>;

  size_t const tables_count_;
  uint32_t const sparse_feature_size_;
  std::vector<uint32_t> table_entries_;
  uint32_t version_;
  Reader reader_;
};

template <typename T, typename Reader, typename Writer>
std::unique_ptr<IPostprocessor<T>>
DLRMPreprocessor<T, Reader, Writer>::load_impl(void *out_ptr,
                                               bool include_tag) {
  Writer out(static_cast<uint8_t *>(out_ptr));

  std::vector<uintptr_t> table_offsets;
  table_offsets.reserve(tables_count_);

  auto rows_per_batch = BUFFER_SIZE / (sizeof(T) * sparse_feature_size_);
  std::vector<T> slice(rows_per_batch * sparse_feature_size_, T{});

  EEngine_t encryption_engine(version_);
  VEngine_t verification_engine(reinterpret_cast<uint64_t>(out_ptr), version_);

  auto slice_offset = 0ULL;
  for (auto row_count : table_entries_) {
    table_offsets.emplace_back(slice_offset);

    // Encrypt single table
    while (row_count > 0) {
      // Fetch data batch
      auto rows_to_read =
          std::min(static_cast<unsigned long>(row_count), rows_per_batch);
      slice.resize(rows_to_read * sparse_feature_size_);

      reader_.read(reinterpret_cast<char *>(slice.data()),
                   pnm::utils::byte_size_of_container(slice));

      // Perform line-by-line encryption
      for (auto it = slice.begin(); it != slice.end();
           it += sparse_feature_size_) {
        pnm::types::uint128_t tag{};
        if (include_tag) {
          tag = verification_engine.generate_mac(it, it + sparse_feature_size_,
                                                 slice_offset);
        }

        auto [_, size] = encryption_engine.encrypt(
            it, it + sparse_feature_size_, it, slice_offset);

        out.write(reinterpret_cast<char *>(&*it),
                  sparse_feature_size_ * sizeof(*it));

        if (include_tag) {
          io_traits<Writer>::write_meta(
              out, reinterpret_cast<const char *>(&tag), sizeof(tag));
        }

        slice_offset += size;
      }

      row_count -= rows_to_read;
    }
  }

  return std::make_unique<DLRMPostprocessor<T>>(
      std::move(encryption_engine), std::move(verification_engine),
      std::move(table_offsets), sparse_feature_size_);
}
} // namespace pnm::sls::secure

#endif // SLS_DLRM_PREPROCESSOR_H
