/*
 * Copyright (C) 2021 Samsung Electronics Co. LTD
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

#ifndef _PNM_SLS_OPERATION_H_
#define _PNM_SLS_OPERATION_H_

#include "embedding_tables.h"

#include "pnmlib/common/compiler.h"
#include "pnmlib/common/views.h"

#include <cstdint>

namespace pnm::operations {

/** @brief Create this and pass to ::SlsRunner
 */
class PNM_API SlsOperation {
public:
  // NOTE: The values of the enum are used as an opcode for operations, so we
  // fix them inplace

  /** @brief SLS operation types. Specifies type of embeddings used in SLS
   * calculation.
   */
  enum class Type : uint8_t {
    /** @brief Treat each sparse feature element(embedding vector coordinate) as
     * float
     */
    Float = 0U,
    /** @brief Treat each sparse feature element(embedding vector coordinate) as
     * uint32
     */
    Uint32 = 2U,
    /** @brief Treat each sparse feature element(embedding vector coordinate) as
     * uint32 and verify calculations result using tags.
     */
    Uint32Tagged = 3U
  };

  /** @brief Default construct the operation. Can't be used in ::SlsRunner,
   * but may be used to delay initialization
   */
  SlsOperation() = default;

  /** @brief SLS Operation constructor
   * Context object, containing information about required operation
   * parameters. Parameters related to embedding tables are set here
   *
   * @param[in] sparse_feature_size Number of elements per single embedding
   * vector/feature/row in embedding tables loaded to PNM device by
   * ::sls_write_rank_data.
   *
   * @param[in] num_table Number of embedding tables
   * loaded to PNM device by ::sls_write_rank_data.
   *
   * @param[in] tables_sizes View of embeddings sizes (number of embedding
   * features per table). Each embedding feature of size
   * feature_size_in_bytes(see ::sls_allocate_tables). This must live at
   * least as much as this object!
   *
   * @param[in] layout Pointer to layout, returned by
   * ::sls_get_tables_layout, determining where on ranks table data should be
   * stored. Must live at least as much as this object!
   * If operation was created with tagged ::Type, then buffer size must be
   * extended to(in general):
   * minibatch_size * num_tables * (sparse_feature_bytes + sizeof(tag)).
   *
   * @param[in] op_type One of ::Type.
   *
   * @throws pnm::error::InvalidArguments
   */
  SlsOperation(uint32_t sparse_feature_size,
               pnm::views::common<const uint32_t> tables_sizes,
               const pnm::memory::EmbeddingTables *tables, Type op_type);

  /** @brief Set parameters related to one run of SLS
   *
   * @param[in] minibatch_size Number of batches per table. Equals to embedding
   * vectors/features number in psum.
   *
   * @param[in] lengths SLS lengths. Number of lookups in @p indices array per
   * table, per minibatch.
   *
   * @param[in] indices SLS indices. Set of indices per table. Each index must
   * not exceed table length.
   *
   * @param psum result PSUM buffer. Array of sparse vectors/features. Number of
   * elements is minibatch_size * num_tables.
   *
   * @throws pnm::error::InvalidArguments
   */
  void set_run_params(uint32_t minibatch_size,
                      pnm::views::common<const uint32_t> lengths,
                      pnm::views::common<const uint32_t> indices,
                      pnm::views::common<uint8_t> psum);

  uint32_t sparse_feature_size() const { return sparse_feature_size_; }
  auto tables_sizes() const { return tables_sizes_; }
  uint32_t minibatch_size() const { return minibatch_size_; }
  auto lengths() const { return lengths_; }
  auto indices() const { return indices_; }
  auto psum() { return psum_; }
  auto psum() const { return pnm::views::view_cast<const uint8_t>(psum_); }
  Type op_type() const { return op_type_; }
  const pnm::memory::EmbeddingTables *tables() const { return tables_; }

private:
  uint32_t sparse_feature_size_{};
  pnm::views::common<const uint32_t> tables_sizes_{};
  uint32_t minibatch_size_{};
  pnm::views::common<const uint32_t> lengths_{};
  pnm::views::common<const uint32_t> indices_{};
  pnm::views::common<uint8_t> psum_{};
  const pnm::memory::EmbeddingTables *tables_ = nullptr;
  Type op_type_{};
};

} // namespace pnm::operations

#endif
