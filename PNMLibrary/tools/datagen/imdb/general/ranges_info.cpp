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

#include "ranges_info.h"

#include "tools/datagen/line_parser.h"

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

RangesInfo::RangesInfo(const std::filesystem::path &path) {
  std::ifstream in(path);
  in.exceptions(std::istream::badbit);
  if (!in) {
    throw std::runtime_error(fmt::format(
        "Unable to open scan range's info file. Reason: {}", strerror(errno)));
  }

  auto parser = create_parser();
  parser.parse_file(in);

  num_requests_ = selectivities_.size();
}

LineParser RangesInfo::create_parser() {
  LineParser lp;

  lp.register_parse_closure("selectivities", [this](auto &iss) {
    std::copy(std::istream_iterator<double>{iss},
              std::istream_iterator<double>{},
              std::back_inserter(this->selectivities_));
  });
  return lp;
}

void RangesInfo::store_to_file(const std::filesystem::path &path) const {
  std::ofstream out(path);
  out.exceptions(std::ofstream::failbit);
  if (!out) {
    throw std::runtime_error(fmt::format(
        "Unable to open scan range's info file for write. Reason: {}",
        strerror(errno)));
  }

  fmt::print(out, "selectivities = {}\n", fmt::join(selectivities(), " "));
}

} // namespace tools::gen::imdb
