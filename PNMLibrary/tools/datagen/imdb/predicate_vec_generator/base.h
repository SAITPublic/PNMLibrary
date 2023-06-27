/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#ifndef IMDB_IPREDICATE_VECS_GENERATOR_H
#define IMDB_IPREDICATE_VECS_GENERATOR_H

#include "tools/datagen/imdb/general/columns_info.h"
#include "tools/datagen/imdb/general/predicate_vecs_info.h"
#include "tools/datagen/imdb/general/predicate_vecs_io.h"

#include "pnmlib/imdb/scan_types.h"

#include <filesystem>
#include <string>

namespace tools::gen::imdb {
/** @brief Interface for predicate vector generators */
class IPredicateVecsGenerator {
public:
  auto create(const PredicateVectorsInfo &predicate_info,
              const ColumnsInfo &column_info) {
    return create_predicate_vecs_impl(predicate_info, column_info);
  }

  void create_and_store(const std::filesystem::path &root,
                        const std::string &prefix,
                        const PredicateVectorsInfo &predicate_info,
                        const ColumnsInfo &column_info) {
    const auto data = create_predicate_vecs_impl(predicate_info, column_info);
    PredicateVectorsIO::store_to_file(data, root, prefix, predicate_info);
  }

  virtual ~IPredicateVecsGenerator() = default;

private:
  virtual Predictors
  create_predicate_vecs_impl(const PredicateVectorsInfo &predicate_info,
                             const ColumnsInfo &column_info) = 0;
};

} // namespace tools::gen::imdb

#endif // IMDB_IPREDICATE_VECS_GENERATOR_H
