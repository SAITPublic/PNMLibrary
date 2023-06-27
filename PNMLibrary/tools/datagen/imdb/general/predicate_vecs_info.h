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
#ifndef IMDB_PREDICATE_VECS_INFO_H
#define IMDB_PREDICATE_VECS_INFO_H

#include "tools/datagen/line_parser.h"

#include "pnmlib/imdb/scan_types.h"

#include <cstddef>
#include <filesystem>
#include <utility>
#include <vector>

namespace tools::gen::imdb {

using namespace pnm::imdb;

/*! \brief Basic information about predicate vectors structure.*/
class PredicateVectorsInfo {
public:
  PredicateVectorsInfo() = default;

  PredicateVectorsInfo(
      std::vector<double> selectivities,
      std::vector<compressed_element_type> predicate_max_values)
      : selectivities_{std::move(selectivities)},
        predicate_max_values_{std::move(predicate_max_values)},
        num_requests_{selectivities_.size()} {
    check_vectors_consistency();
  }

  explicit PredicateVectorsInfo(const std::filesystem::path &path);

  const auto &selectivities() const { return selectivities_; }

  const auto &predicate_max_values() const { return predicate_max_values_; }

  auto num_requests() const { return num_requests_; }

  void store_to_file(const std::filesystem::path &path) const;

private:
  LineParser create_parser();

  void check_vectors_consistency() const;

  std::vector<double> selectivities_;

  std::vector<compressed_element_type> predicate_max_values_;

  size_t num_requests_{0};
};

} // namespace tools::gen::imdb

#endif // IMDB_PREDICATE_VECS_INFO_H
