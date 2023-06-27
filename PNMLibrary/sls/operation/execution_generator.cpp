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

#include "execution_generator.h"

#include "core/device/sls/rank_memory.h"

#include "common/log.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace {
void print_overflowed_table_data(uint32_t table_id, uint32_t batch_lookups) {
  pnm::log::debug("Table id: {}", table_id);
  pnm::log::debug("Table current batch lookups: {}", batch_lookups);
}
} // namespace

namespace pnm::sls {

void Execution::commit_table_batches(std::vector<uint32_t> &tables_batches) {
  TableData &table_data = tables_data_.back();
  table_data.batch_offset_ = tables_batches[table_data.table_id_];
  tables_batches[table_data.table_id_] += table_data.table_batches_chunk_;
}

void Execution::add_batch(uint32_t t_id, uint32_t b_lookups,
                          std::vector<uint32_t> &tables_batches) {
  assert(b_lookups != 0);
  if (tables_data_.empty() || tables_data_.back().table_id_ != t_id) {
    if (!tables_data_.empty()) {
      commit_table_batches(tables_batches);
    }
    tables_data_.emplace_back(t_id, 0);
  }
  ++tables_data_.back().table_batches_chunk_;
  ++total_batches_in_chunks_;
  total_lookups_ += b_lookups;
}

void ExecutionGenerator::print_current_exec() const {
  pnm::log::debug("Current execution info:");
  pnm::log::debug("\tTables number: {}", current_exec_.tables_data_.size());
  pnm::log::debug("\tTotal lookups: {}", current_exec_.total_lookups_);
  pnm::log::debug("\tTotal batches in chunks: {}",
                  current_exec_.total_batches_in_chunks_);
  const size_t psum_size =
      current_exec_.total_batches_in_chunks_ * sparse_feature_in_bytes_;
  pnm::log::debug("\tPsum size in bytes: {}", psum_size);
}

bool ExecutionGenerator::try_add_batch(uint32_t table_id,
                                       uint32_t batch_lookups) {
  assert(batch_lookups <= max_inst_in_buf_);
  // Batch will overflow instruction buffer
  if (current_exec_.total_lookups_ + batch_lookups > max_inst_in_buf_) {
    pnm::log::debug("Instruction buffer overflow");
    print_overflowed_table_data(table_id, batch_lookups);
    print_current_exec();
    current_exec_.commit_table_batches(tables_batches_);
    return false;
  }

  // Batch will overflow psum buiffer
  if (current_exec_.total_batches_in_chunks_ + 1 > max_features_in_buf_) {
    pnm::log::debug("Psum buffer overflow");
    print_overflowed_table_data(table_id, batch_lookups);
    print_current_exec();
    current_exec_.commit_table_batches(tables_batches_);
    return false;
  }

  // Batch doesn't overflow
  current_exec_.add_batch(table_id, batch_lookups, tables_batches_);
  return true;
}

void ExecutionGenerator::commit_execution(uint8_t pack) {
  current_exec_.psum_size_ =
      current_exec_.total_batches_in_chunks_ * sparse_feature_in_bytes_;
  assert(current_exec_.total_lookups_ > 0);
  assert(current_exec_.total_batches_in_chunks_ > 0);
  assert(current_exec_.total_lookups_ <= max_inst_in_buf_);
  assert(current_exec_.total_batches_in_chunks_ <= max_features_in_buf_);
  exec_packs_[pack].emplace_back(std::move(current_exec_));
  // Put object into well-specified state after move
  current_exec_ = {};
}

void ExecutionGenerator::generate(size_t mini_batch_size, size_t num_tables,
                                  const uint32_t *lengths) {
  assert(max_inst_in_buf_ > 0);
  assert(max_features_in_buf_ > 0);
  assert(sparse_feature_in_bytes_ > 0);
  assert(mini_batch_size > 0);
  assert(num_tables > 0);
  assert(lengths);

  tables_batches_.resize(num_tables);

  // Leave allocated memory (if any) for further usage
  for (auto &execs : exec_packs_) {
    execs.clear();
  }

  for (const auto pack : device::RankMemory::pack_identifiers()) {
    for (const auto &obj : tables_layout_[pack]) {
      const auto current_table_batch_offset = obj.id() * mini_batch_size;
      const uint32_t *current_batch_lookups;

      assert(current_table_batch_offset + mini_batch_size - 1 <
             num_tables * mini_batch_size);
      // Get batch lookups from lengths,
      // this array length = num_tables * mini_batch_size
      current_batch_lookups = lengths + current_table_batch_offset;

      for (auto batch_idx = 0U; batch_idx < mini_batch_size; ++batch_idx) {
        // If can't add new batch, i.e either psum or instruction device buffers
        // overflowed - then commit execution
        if (!try_add_batch(obj.id(), *current_batch_lookups)) {
          commit_execution(pack);
          // Here table batches get split into different executions
          current_exec_.add_batch(obj.id(), *current_batch_lookups,
                                  tables_batches_);
        }

        ++current_batch_lookups;
      }
    }
    // Commit last execution if anything left uncommited for compute unit
    if (current_exec_.total_lookups_) {
      current_exec_.commit_table_batches(tables_batches_);
      commit_execution(pack);
    }
  }
}
} // namespace pnm::sls
