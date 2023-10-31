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

#include "sls/operation/param_checks.h"

#include "pnmlib/sls/embedding_tables.h"
#include "pnmlib/sls/operation.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/views.h"

#include <cstdint>

pnm::operations::SlsOperation::SlsOperation(
    uint32_t sparse_feature_size,
    pnm::views::common<const uint32_t> tables_sizes,
    const pnm::memory::EmbeddingTables *tables,
    pnm::operations::SlsOperation::Type op_type)
    : sparse_feature_size_(sparse_feature_size), tables_sizes_(tables_sizes),
      tables_(tables), op_type_(op_type) {
  if (!tables_) {
    throw pnm::error::InvalidArguments("Tables do not exist.");
  }

  pnm::sls::check_sparse_feature_size(sparse_feature_size);
}

void pnm::operations::SlsOperation::set_run_params(
    uint32_t minibatch_size, pnm::views::common<const uint32_t> lengths,
    pnm::views::common<const uint32_t> indices,
    pnm::views::common<uint8_t> psum) {
  minibatch_size_ = minibatch_size;
  lengths_ = lengths;
  indices_ = indices;
  psum_ = psum;

  pnm::sls::check_run_params(*this);
}
