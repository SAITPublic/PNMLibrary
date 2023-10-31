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

#ifndef _SLS_PACKS_DATA_H_
#define _SLS_PACKS_DATA_H_

#include "sls/operation/execution_generator.h"

#include "core/device/sls/rank_memory.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace pnm::operations {
class SlsOperation;
} // namespace pnm::operations

namespace pnm::sls {

class ExecutionGenerator;

/** @brief Class for calculation of lenghts, indices, psum
  and tag addresses for each table in each execution.
  Does not depend on tables order in execution pack.
*/
class ExecutionPacksData {
public:
  explicit ExecutionPacksData(const pnm::operations::SlsOperation &sls_op);

  void init(const ExecutionGenerator &exec_gen);

  const auto &lengths(uint64_t pack_idx, uint64_t exec_idx) const {
    return length_exec_[pack_idx][exec_idx];
  }

  const auto &indices(uint64_t pack_idx, uint64_t exec_idx) const {
    return index_exec_[pack_idx][exec_idx];
  }

  const auto &tags(uint64_t pack_idx, uint64_t exec_idx) const {
    return tags_addr_[pack_idx][exec_idx];
  }

  auto &tags(uint64_t pack_idx, uint64_t exec_idx) {
    return tags_addr_[pack_idx][exec_idx];
  }

  const auto &psum(uint64_t pack_idx, uint64_t exec_idx) const {
    return psum_addr_[pack_idx][exec_idx];
  }

  auto &psum(uint64_t pack_idx, uint64_t exec_idx) {
    return psum_addr_[pack_idx][exec_idx];
  }

private:
  void init_pack_data(uint64_t pack_idx, size_t size);
  void init_pack_exec_data(uint64_t pack_idx, uint64_t exec_idx, size_t size);
  void init_tables_indexing_data();
  void init_data_initializers();

  void fill_table_data(const Execution::TableData &table_data,
                       uint64_t pack_idx, uint64_t exec_idx,
                       uint64_t table_idx);

  /**
   * Calculates addreses of lengths, indices, psum and tags for each table
   * chunk in each pack execution. Number of calculated addresses can be larger
   * than number of tables in case of psum/instruction buffer overflows
   * Resulting addreses are:
   *  lengths - current table batches chunk first batch
   *  length indices - current table batches chunk first batch index
   *  psum - current table batches chunk first batch psum output
   *  tag - current table batches chunk first batch tag
   *
   * This allows to generate valid trace for each table batches chunk
   * and write results to valid table batches chunk location
   */
  void fill_packs_data(const ExecutionGenerator &exec_gen);

  using RankMemory = device::RankMemory;
  using PsumPacksData =
      RankMemory::packs_array<std::vector<std::vector<uint8_t *>>>;
  using TagsPacksData =
      RankMemory::packs_array<std::vector<std::vector<uint8_t *>>>;
  using LengthsPacksData =
      RankMemory::packs_array<std::vector<std::vector<const uint32_t *>>>;
  using IndicesPacksData =
      RankMemory::packs_array<std::vector<std::vector<const uint32_t *>>>;
  using DataInitializer = std::function<void(const Execution::TableData &,
                                             uint64_t, uint64_t, uint64_t)>;

  PsumPacksData psum_addr_;
  TagsPacksData tags_addr_;
  LengthsPacksData length_exec_;
  IndicesPacksData index_exec_;

  const operations::SlsOperation &sls_op_;

  std::vector<DataInitializer> data_initializers_;

  std::vector<const uint32_t *> indices_offsets_;
  std::vector<const uint32_t *> lengths_offsets_;

  bool is_tagged_{};
};

} // namespace pnm::sls

#endif
