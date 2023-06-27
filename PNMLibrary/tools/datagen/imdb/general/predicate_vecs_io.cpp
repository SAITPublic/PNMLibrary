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

#include "predicate_vecs_io.h"

#include "predicate_vecs_info.h"

#include "common/mapped_file.h"

#include "pnmlib/imdb/bit_containers.h"
#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/common/views.h"

#include <fmt/core.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>

namespace tools::gen::imdb {

using namespace pnm::imdb;

void PredicateVectorsIO::store_to_file(
    const Predictors &predictors, const std::filesystem::path &root,
    const std::string &filename_prefix,
    const PredicateVectorsInfo &predicate_info) {
  const auto data_path = root / (filename_prefix + predicate_vec_suffix_bin);
  const auto info_path = root / (filename_prefix + predicate_vec_suffix_info);
  predicate_info.store_to_file(info_path);

  std::ofstream out(data_path, std::ios::out | std::ios::binary);
  out.exceptions(std::ofstream::badbit);
  if (out) {
    for (const auto &predicate_vector : predictors) {
      out.write(
          reinterpret_cast<const char *>(predicate_vector.container().data()),
          predicate_vector.bytes());
    }
  } else {
    throw std::runtime_error(fmt::format(
        "Unable to open predicate vector file in write mode. Reason: {}",
        strerror(errno)));
  }
}

Predictors
PredicateVectorsIO::load_from_file(const PredicateVectorsMmap &mmapped_vecs) {
  Predictors predictors;
  predictors.reserve(mmapped_vecs.info.num_requests());

  size_t expected_size = 0;
  for (auto max_value : mmapped_vecs.info.predicate_max_values()) {
    predictors.emplace_back(static_cast<uint64_t>(max_value) + 1);
    expected_size += predictors.back().bytes();
  }

  if (expected_size != mmapped_vecs.mmapped_file.size()) {
    throw std::runtime_error(fmt::format(
        "Expected predicate vectors file size ({} bytes) doesn't match "
        "actual size ({} bytes)",
        expected_size, mmapped_vecs.mmapped_file.size()));
  }
  const auto view =
      mmapped_vecs.mmapped_file.get_view<compressed_element_type>();
  const auto *begin_it = view.begin();
  for (auto &predictor : predictors) {
    auto &container = predictor.container();
    const auto *const end_it = begin_it + container.size();
    std::copy(begin_it, end_it, container.begin());
    begin_it = end_it;
  }

  return predictors;
}

} // namespace tools::gen::imdb
