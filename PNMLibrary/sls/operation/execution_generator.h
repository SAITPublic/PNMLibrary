/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or
 * computer language in any form
 * by any means, electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung
 * Electronics.
 */

#ifndef SLS_EXECUTION_GENERATOR_H
#define SLS_EXECUTION_GENERATOR_H

#include "core/device/sls/rank_memory.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace pnm::sls {

class Execution {
public:
  struct TableData {
    // Constructor to use emplace_back
    TableData(uint32_t table_id, uint32_t table_batches_chunk)
        : table_id_(table_id), table_batches_chunk_(table_batches_chunk) {}

    //! Global table id
    uint32_t table_id_;
    //! Chunk of table batches in range [1, mini_batch_size]
    uint32_t table_batches_chunk_;
    //! Current table batch chunk offset from first batch in range [0,
    //! mini_batch_size - 1]
    uint32_t batch_offset_;
  };

  void add_batch(uint32_t t_id, uint32_t b_lookups,
                 std::vector<uint32_t> &tables_batches);

  void commit_table_batches(std::vector<uint32_t> &tables_batches);

  [[nodiscard]] auto get_num_lookups() const noexcept { return total_lookups_; }

  [[nodiscard]] auto get_num_batches() const noexcept {
    return total_batches_in_chunks_;
  }

  [[nodiscard]] auto get_psum_read_size() const noexcept { return psum_size_; }

  [[nodiscard]] const auto &get_tables_data() const noexcept {
    return tables_data_;
  }

  TableData &get_last_table() noexcept { return tables_data_.back(); }

private:
  friend class ExecutionGenerator;
  //! Total data of tables batches/lookups fitting in device buffers
  std::vector<TableData> tables_data_;
  //! Total lookups of all table data in execution
  size_t total_lookups_{};
  //! Total batches of all table data in execution
  size_t total_batches_in_chunks_{};
  //! Total psum buffer space consumed by execution
  size_t psum_size_{};
};

class ExecutionGenerator {
public:
  ExecutionGenerator() = default;

  ExecutionGenerator(device::RankMemory::packs_to_objects tables_layout,
                     size_t max_inst_in_buf, size_t max_features_in_buf,
                     size_t sparse_feature_bytes) {
    init(std::move(tables_layout), max_inst_in_buf, max_features_in_buf,
         sparse_feature_bytes);
  }

  void init(device::RankMemory::packs_to_objects tables_layout,
            size_t max_inst_in_buf, size_t max_features_in_buf,
            size_t sparse_feature_in_bytes) noexcept {
    tables_layout_ = std::move(tables_layout);
    max_inst_in_buf_ = max_inst_in_buf;
    max_features_in_buf_ = max_features_in_buf;
    sparse_feature_in_bytes_ = sparse_feature_in_bytes;
  }

  [[nodiscard]] const auto &get_pack_execs(uint8_t pack) const {
    return exec_packs_[pack];
  }

  //! \brief Generates execution from lengths array, which is of size num_tables
  //! * mini_batch_size
  void generate(size_t mini_batch_size, size_t num_tables,
                const uint32_t *lengths);

private:
  void commit_execution(uint8_t pack);
  void print_current_exec() const;
  [[nodiscard]] bool try_add_batch(uint32_t table_id, uint32_t batch_lookups);

  //! Object used to process each exec at the moment of time in pack
  Execution current_exec_;
  /* Executions storage, executions are not bound to the compute unit
   * physically, however the way tables are placed in compute units determines
   * certain sets of executions i.e. packs of executions. In
   * SLS_ALLOC_REPLICATE_ALL memory policy case we have just only 1 pack and we
   * can run 1 executions set on any compute unit. It's not final, because for
   * boost we may want to generate more packs to be able to run them in
   * parallel. In SLS_ALLOC_DISTRIBUTE_ALL it's 4 packs == NUM_OF_RANKS. In
   * SLS_ALLOC_DISTRIBUTE_HALF it will be 2 packs. But in common case it's not
   * determined by the used compute unit's number, and can be more than even
   * NUM_OF_PACKS. In fact, packs number is executions combinations number.
   */
  device::RankMemory::packs_array<std::vector<Execution>> exec_packs_;
  device::RankMemory::packs_to_objects tables_layout_;
  /** Tracks amount of commited batches per table batch chunk.
   *  Used to calculate batch chunk offset inside single table batch
   *  during executions generation.
   */
  std::vector<uint32_t> tables_batches_;
  //! Maximum number of instructions that fit into instruction buffer
  size_t max_inst_in_buf_{};
  //! Maximum amount of sparse vectors that fit into psum buffer
  size_t max_features_in_buf_{};
  //! Size of single sparse vector in bytes
  size_t sparse_feature_in_bytes_{};
};

} // namespace pnm::sls

#endif
