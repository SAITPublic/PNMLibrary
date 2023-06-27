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

#include "predicate_vecs_info.h"

#include "tools/datagen/line_parser.h"

#include "pnmlib/imdb/scan_types.h"

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <istream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace tools::gen::imdb {

using namespace pnm::imdb;

PredicateVectorsInfo::PredicateVectorsInfo(const std::filesystem::path &path) {
  std::ifstream in(path);
  in.exceptions(std::istream::badbit);
  if (!in) {
    throw std::runtime_error(
        fmt::format("Unable to open predicate vector's info file. Reason: {}",
                    strerror(errno)));
  }

  auto parser = create_parser();
  parser.parse_file(in);

  check_vectors_consistency();

  num_requests_ = selectivities_.size();
}

LineParser PredicateVectorsInfo::create_parser() {
  LineParser lp;
  lp.register_parse_closure("selectivities", [this](auto &iss) {
    std::copy(std::istream_iterator<double>{iss},
              std::istream_iterator<double>{},
              std::back_inserter(this->selectivities_));
  });
  lp.register_parse_closure("predicate_max_values", [this](auto &iss) {
    std::copy(std::istream_iterator<compressed_element_type>{iss},
              std::istream_iterator<compressed_element_type>{},
              std::back_inserter(this->predicate_max_values_));
  });

  return lp;
}

void PredicateVectorsInfo::store_to_file(
    const std::filesystem::path &path) const {
  std::ofstream out(path);
  out.exceptions(std::ofstream::failbit);
  if (!out) {
    throw std::runtime_error(fmt::format(
        "Unable to open predicate vector's info file for write. Reason: {}",
        strerror(errno)));
  }

  fmt::print(out, "selectivities = {}\n", fmt::join(selectivities(), " "));
  fmt::print(out, "predicate_max_values = {}\n",
             fmt::join(predicate_max_values(), " "));
}

void PredicateVectorsInfo::check_vectors_consistency() const {
  if (selectivities_.size() != predicate_max_values_.size()) {
    throw std::runtime_error(
        "Selectivities and predicate max values vector sizes don't match");
  }
}

} // namespace tools::gen::imdb
