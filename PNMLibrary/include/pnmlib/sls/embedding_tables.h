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

#ifndef SLS_TABLES_LAYOUT_H
#define SLS_TABLES_LAYOUT_H

#include "pnmlib/core/buffer.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/memory_object.h"

#include "pnmlib/common/compiler.h"
#include "pnmlib/common/views.h"

#include <linux/sls_resources.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace pnm::memory {

/** @brief Class to handle embedding tables
 *
 * The class create, fill and destroy embedding tables for SLS operations.
 *
 */
class PNM_API EmbeddingTables {
public:
  template <typename... Args> static auto create(Args &&...args) {
    return std::make_unique<EmbeddingTables>(std::forward<Args>(args)...);
  }

  /** @brief Create object that handle embedding tables and copy user data to it
   *
   * The ctor will allocate tables_sizes.size() buffers with sizes defined by
   * table_size and row_size and bind user data to the respective buffer. After
   * allocation, the copy to device will perform.
   *
   * @tparam T the type of user data
   * @param host view over the user data
   * @param table_sizes tables size
   * @param row_size size of table's row in bytes
   * @param ctx pnm context
   * @param option the allocation policy, AUTO, REPLICATE or DISTRIBUTE
   */
  template <typename T>
  EmbeddingTables(pnm::views::common<const T> host,
                  const std::vector<uint32_t> &table_sizes, uint64_t row_size,
                  const ContextHandler &ctx,
                  sls_user_preferences option = SLS_ALLOC_AUTO)
      : EmbeddingTables(table_sizes, row_size, ctx, option) {
    bind_tables_data(pnm::views::view_cast<const uint8_t>(host));
  }

  /** @brief Create object that handle embedding tablse
   *
   * The ctor will allocate tables_sizes.size() buffers with sizes defined by
   * table_size and row_size.
   *
   * @param table_sizes tables size
   * @param row_size size of table's row in bytes
   * @param ctx pnm context
   * @param option the allocation policy, AUTO, REPLICATE or DISTRIBUTE
   */
  EmbeddingTables(const std::vector<uint32_t> &table_sizes, uint64_t row_size,
                  const ContextHandler &ctx,
                  sls_user_preferences option = SLS_ALLOC_AUTO);

  /** @brief Bind and copy user data to device memory
   *
   * @param host view over the user data
   */
  void bind_tables_data(pnm::views::common<const uint8_t> host);

  const auto &buffers() const { return buffers_; }

  const auto &layout() const { return layout_; }

private:
  void init_buffers(const std::vector<uint32_t> &table_sizes,
                    uint64_t row_bytes, const ContextHandler &ctx,
                    sls_user_preferences option);

  void transfer_data(pnm::views::common<const uint8_t> host);

  void construct_layout();

  std::vector<Buffer<uint8_t>> buffers_{};
  std::vector<std::vector<MemoryObject>> layout_;
};

using EmbeddingTablesHandler = std::unique_ptr<EmbeddingTables>;

} // namespace pnm::memory

#endif /* SLS_TABLES_LAYOUT_H */
