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

#include "columns_info.h"

#include "tools/datagen/line_parser.h"

#include <fmt/core.h>
#include <fmt/ostream.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <istream>
#include <stdexcept>
#include <string>

namespace tools::gen::imdb {

void ColumnsInfo::load_from_file(const std::filesystem::path &path) {
  std::ifstream in(path);
  in.exceptions(std::istream::badbit);
  if (!in) {
    throw std::runtime_error(fmt::format(
        "Unable to open columns's info file. Reason: {}", strerror(errno)));
  }

  auto parser = create_parser();
  parser.parse_file(in);
}

LineParser ColumnsInfo::create_parser() {
  LineParser lp;

  lp.register_parse_closure(
      "entry_bit_size", [this](auto &iss) { iss >> this->entry_bit_size_; });

  lp.register_parse_closure("entry_count",
                            [this](auto &iss) { iss >> this->entry_count_; });

  return lp;
}

void ColumnsInfo::store_to_file(const std::filesystem::path &path) const {
  std::ofstream out(path);
  out.exceptions(std::ofstream::badbit | std::ofstream::failbit);
  if (!out) {
    throw std::runtime_error(
        fmt::format("Unable to open column's info file for write. Reason: {}",
                    strerror(errno)));
  }

  fmt::print(out, "entry_bit_size = {}\n", entry_bit_size());
  fmt::print(out, "entry_count = {}\n", entry_count());
}

} // namespace tools::gen::imdb
