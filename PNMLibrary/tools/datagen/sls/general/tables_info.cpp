/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#include "tables_info.h"

#include "tools/datagen/line_parser.h"

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <istream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace tools::gen::sls {

void TablesInfo::load_from_file(const std::filesystem::path &path) {
  std::ifstream in(path);
  in.exceptions(std::istream::badbit);
  if (!in) {
    throw std::runtime_error("Unable to open table's info file.");
  }

  auto parser = create_parser();

  std::string var_line;
  while (std::getline(in, var_line)) {
    if (!var_line.empty()) {
      parser.parse_line(var_line);
    }
  }
}

LineParser TablesInfo::create_parser() {
  LineParser lp;
  lp.register_parse_closure("tables_count",
                            [this](auto &iss) { iss >> this->num_tables_; });

  lp.register_parse_closure("table_columns",
                            [this](auto &iss) { iss >> this->cols_; });

  lp.register_parse_closure("table_rows", [this](auto &iss) {
    std::copy(std::istream_iterator<size_t>{iss},
              std::istream_iterator<size_t>{},
              std::back_inserter(this->table_rows_));
  });
  return lp;
}

void TablesInfo::store_to_file(const std::filesystem::path &path) const {
  std::ofstream out(path);
  if (!out) {
    throw std::runtime_error("Unable to open table's info file for write.");
  }
  out.exceptions(std::ofstream::badbit | std::ofstream::failbit);

  fmt::print(out, "tables_count = {}\n", num_tables_);
  fmt::print(out, "table_columns = {}\n", cols_);
  fmt::print(out, "table_rows = {}\n", fmt::join(rows(), " "));
}

} // namespace tools::gen::sls
