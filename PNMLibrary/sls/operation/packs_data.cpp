/*
 *  Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 *  This software is proprietary of Samsung Electronics.
 *  No part of this software, either material or conceptual may be copied or
 *  distributed, transmitted, transcribed, stored in a retrieval system or
 *  translated into any human or computer language in any form by any means,
 *  electronic, mechanical, manual or otherwise, or disclosed to third parties
 *  without the express written permission of Samsung Electronics.
 */

#include "sls/operation/packs_data.h"

#include "sls/operation/execution_generator.h"

#include "common/topology_constants.h"

#include "pnmlib/sls/operation.h"

#include "pnmlib/common/128bit_math.h"

#include <cstddef>
#include <cstdint>
#include <numeric>

namespace pnm::sls {

ExecutionPacksData::ExecutionPacksData(
    const pnm::operations::SlsOperation &sls_op)
    : sls_op_(sls_op),
      is_tagged_(sls_op_.op_type() ==
                 pnm::operations::SlsOperation::Type::Uint32Tagged) {}

void ExecutionPacksData::init(const ExecutionGenerator &exec_gen) {
  init_data_initializers();
  fill_packs_data(exec_gen);
}

void ExecutionPacksData::init_pack_data(uint64_t pack_idx, size_t size) {
  length_exec_[pack_idx].resize(size);
  index_exec_[pack_idx].resize(size);
  psum_addr_[pack_idx].resize(size);

  if (is_tagged_) {
    tags_addr_[pack_idx].resize(size);
  }
}

void ExecutionPacksData::init_pack_exec_data(uint64_t pack_idx,
                                             uint64_t exec_idx, size_t size) {
  length_exec_[pack_idx][exec_idx].resize(size);
  index_exec_[pack_idx][exec_idx].resize(size);
  psum_addr_[pack_idx][exec_idx].resize(size);

  if (is_tagged_) {
    tags_addr_[pack_idx][exec_idx].resize(size);
  }
}

void ExecutionPacksData::init_tables_indexing_data() {
  const uint64_t num_tables = sls_op_.tables_sizes().size();
  indices_offsets_.resize(num_tables);
  lengths_offsets_.resize(num_tables);

  // Since the order of tables in execution is uspecified
  // and number of lookups per batch can be arbitrary number
  // we accumulate lookups to find valid index offset for each table.
  // So during packs iteration the only lengths reduction required
  // is equal to particular table batch chunk offset
  // Doing so in random order will introduce significant overhead, since
  // for each table full reduction of lengths up to current table batch is
  // required.
  uint64_t index_offset = 0ULL;
  uint64_t batch_offset = 0ULL;
  const auto *lengths = sls_op_.lengths().data();
  const auto *indices = sls_op_.indices().data();
  for (uint64_t table_idx = 0ULL; table_idx < num_tables; ++table_idx) {
    indices_offsets_[table_idx] = indices + index_offset;
    lengths_offsets_[table_idx] = lengths + batch_offset;

    batch_offset += sls_op_.minibatch_size();
    index_offset += std::accumulate(
        lengths_offsets_[table_idx],
        lengths_offsets_[table_idx] + sls_op_.minibatch_size(), 0);
  }
}

void ExecutionPacksData::init_data_initializers() {
  init_tables_indexing_data();

  // Calculation of table batch chunk indices address.
  // Uses previously calculated table indices address
  // and calculates current batch chunk indices address by reducing
  // preceding batches lengths(if any) and adding it to table indices address
  data_initializers_.emplace_back([this](const Execution::TableData &table_data,
                                         uint64_t pack_idx, uint64_t exec_idx,
                                         uint64_t table_idx) {
    index_exec_[pack_idx][exec_idx][table_idx] =
        indices_offsets_[table_data.table_id_] +
        std::accumulate(lengths_offsets_[table_data.table_id_],
                        lengths_offsets_[table_data.table_id_] +
                            table_data.batch_offset_,
                        0);
  });

  // Calculation of table batch chunk lengths address.
  // Uses previously calculated table lenghts address
  data_initializers_.emplace_back([this](const Execution::TableData &table_data,
                                         uint64_t pack_idx, uint64_t exec_idx,
                                         uint64_t table_idx) {
    length_exec_[pack_idx][exec_idx][table_idx] =
        lengths_offsets_[table_data.table_id_] + table_data.batch_offset_;
  });

  // Calculation of table batch psum output buffer adress
  const auto sp_feature_size =
      sls_op_.sparse_feature_size() * pnm::sls::device::topo().DataSize;
  const uint64_t data_size = sls_op_.minibatch_size() * sp_feature_size;
  auto *psum = const_cast<uint8_t *>(sls_op_.psum().data());
  data_initializers_.emplace_back([this, sp_feature_size, data_size,
                                   psum](const Execution::TableData &table_data,
                                         uint64_t pack_idx, uint64_t exec_idx,
                                         uint64_t table_idx) {
    psum_addr_[pack_idx][exec_idx][table_idx] =
        psum + table_data.table_id_ * data_size +
        table_data.batch_offset_ * sp_feature_size;
  });

  // Calculation of table batch tags output buffer address
  if (is_tagged_) {
    const auto tag_size = sizeof(pnm::types::uint128_t);
    const uint64_t tags_per_table_size = sls_op_.minibatch_size() * tag_size;
    auto *tags = psum + sls_op_.lengths().size() * sp_feature_size;
    data_initializers_.emplace_back([this, tags, tags_per_table_size](
                                        const Execution::TableData &table_data,
                                        uint64_t pack_idx, uint64_t exec_idx,
                                        uint64_t table_idx) {
      tags_addr_[pack_idx][exec_idx][table_idx] =
          tags + table_data.table_id_ * tags_per_table_size +
          table_data.batch_offset_ * tag_size;
    });
  }
}

void ExecutionPacksData::fill_table_data(const Execution::TableData &table_data,
                                         uint64_t pack_idx, uint64_t exec_idx,
                                         uint64_t table_idx) {
  for (const auto &data_initializer : data_initializers_) {
    data_initializer(table_data, pack_idx, exec_idx, table_idx);
  }
}

/**
 * Calculates addreses of lengths, indices, psum and tags for each table
 * chunk in each pack execution. Number of calculated addresses can be larger
 * than number of tables in case of psum/instruction bufer overflows Resulting
 * addreses are: lengths - current table batches chunk first batch length
 *  indices - current table batches chunk first batch index
 *  psum - current table batches chunk first batch psum output
 *  tag - current table batches chunk first batch tag
 *
 * This allows to generate valid trace for each table batches chunk
 * and write results to valid table batches chunk location
 */
void ExecutionPacksData::fill_packs_data(const ExecutionGenerator &exec_gen) {
  for (const auto pack : RankMemory::pack_identifiers()) {
    const auto &execs = exec_gen.get_pack_execs(pack);
    const size_t execs_num = execs.size();
    if (execs_num == 0) {
      continue;
    }

    init_pack_data(pack, execs_num);

    for (uint64_t exec_idx = 0ULL; exec_idx < execs.size(); ++exec_idx) {
      const auto &tables_data = execs[exec_idx].get_tables_data();
      init_pack_exec_data(pack, exec_idx, tables_data.size());
      for (uint64_t table_idx = 0ULL; table_idx < tables_data.size();
           ++table_idx) {
        fill_table_data(tables_data[table_idx], pack, exec_idx, table_idx);
      }
    }
  }

  // We don't need this data after all tables've been processed
  indices_offsets_.clear();
  lengths_offsets_.clear();
}

} // namespace pnm::sls
