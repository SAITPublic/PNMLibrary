#include "param_checks.h"

#include "core/device/sls/base.h"
#include "core/device/sls/utils/constants.h"

#include "common/make_error.h"

#include "pnmlib/sls/operation.h"

#include "pnmlib/common/definitions.h"
#include "pnmlib/common/error.h"

#include <fmt/format.h>

#include <linux/sls_resources.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace pnm::sls {

namespace {

// NOTE: Unused whenever building in release
[[maybe_unused]] void check_indices(const pnm::operations::SlsOperation &op) {
  for (uint32_t table_id = 0; table_id < op.tables_sizes().size(); ++table_id) {
    // Max number of vectors per table
    const auto tables_sizes = op.tables_sizes()[table_id];
    // Offset of per batch table data
    const auto table_data_offset = table_id * op.minibatch_size();
    for (uint32_t batch_id = 0; batch_id < op.minibatch_size(); ++batch_id) {
      // Current table batch offest
      const auto table_batch_data_offset = table_data_offset + batch_id;
      // Lookup length for per table per batch
      const auto data_lookup_length = op.lengths()[table_batch_data_offset];

      for (uint32_t lookup_id = 0; lookup_id < data_lookup_length;
           ++lookup_id) {
        const auto sparse_vector_idx =
            op.indices()[table_batch_data_offset + lookup_id];
        if (sparse_vector_idx >= tables_sizes) {
          throw pnm::error::make_inval(
              "Per batch sparse index. Table index: {}, batch "
              "index: {}, sparse index: {}. Max lookup index: {}.",
              table_id, batch_id, sparse_vector_idx, tables_sizes - 1);
        }
      }
    }
  }
}

} // namespace

void check_sparse_feature_size(uint32_t sparse_feature_size) {
  static const std::array<uint32_t, 3> sparse_feature_sizes{16, 32, 64};
  const auto *res =
      std::find(std::begin(sparse_feature_sizes),
                std::end(sparse_feature_sizes), sparse_feature_size);
  if (res == std::end(sparse_feature_sizes)) {
    throw pnm::error::make_inval(
        "Sparse feature size: {}. Available sizes: {}.", sparse_feature_size,
        fmt::join(sparse_feature_sizes, ", "));
  }
}

void check_device_memory_limits(const device::BaseDevice &device,
                                size_t objects_size_in_bytes,
                                sls_user_preferences preference) {

  auto max_mem_size = preference == SLS_ALLOC_DISTRIBUTE_ALL
                          ? device.get_base_memory_size()
                          : device.get_min_block_size(SLS_BLOCK_BASE);
  if (objects_size_in_bytes == 0 || objects_size_in_bytes > max_mem_size) {
    throw pnm::error::make_oom(
        "Objects total size is too big: {}. Available objects memory: {}.",
        objects_size_in_bytes, max_mem_size);
  }

  const auto min_psum_buf_size_bytes =
      device.get_min_block_size(SLS_BLOCK_PSUM);
  if (device::psum_single_buf_size() == 0 ||
      min_psum_buf_size_bytes < device::psum_single_buf_size()) {
    throw pnm::error::make_oom(
        "Psum buffer size is invalid: {}. Available psum memory: {}.",
        device::psum_single_buf_size(), min_psum_buf_size_bytes);
  }

  const auto min_instr_buf_size_bytes =
      device.get_min_block_size(SLS_BLOCK_INST);
  if (device::inst_single_buf_size() == 0 ||
      min_instr_buf_size_bytes < device::inst_single_buf_size()) {
    throw pnm::error::make_oom(
        "Instruction buffer size is invalid: {}. Available instruction "
        "memory: {}.",
        device::inst_single_buf_size(), min_instr_buf_size_bytes);
  }
}

void check_run_params(const pnm::operations::SlsOperation &op) {
  if (op.minibatch_size() == 0) {
    throw pnm::error::make_inval("Minibatch size: {}.", op.minibatch_size());
  }

  if (op.lengths().empty()) {
    throw pnm::error::InvalidArguments("Lookup lengths can't be empty.");
  }

  if (op.indices().empty()) {
    throw pnm::error::InvalidArguments("Lookup indices can't be empty.");
  }

  if (op.psum().empty()) {
    throw pnm::error::InvalidArguments("Psum results can't be empty.");
  }

  // Might be expensive, so don't run in release
  if constexpr (DEBUG) {
    check_indices(op);
  }
}
} // namespace pnm::sls
