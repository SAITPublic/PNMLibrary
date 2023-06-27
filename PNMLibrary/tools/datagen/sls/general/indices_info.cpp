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

#include "indices_info.h"

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

IndicesInfo::IndicesInfo(const std::filesystem::path &path) {
  std::ifstream in(path);
  in.exceptions(std::istream::badbit);
  if (!in) {
    static const thread_local std::string error_message =
        std::string{"Unable to open table's info file. Reason: "} +
        strerror(errno);
    throw std::runtime_error(error_message.c_str());
  }

  auto parser = create_parser();
  std::string var_line;
  while (std::getline(in, var_line)) {
    if (!var_line.empty()) {
      parser.parse_line(var_line);
    }
  }

  num_requests_ = lengths_.size();
}

LineParser IndicesInfo::create_parser() {
  LineParser lp;
  lp.register_parse_closure("lengths", [this](auto &iss) {
    std::copy(std::istream_iterator<size_t>{iss},
              std::istream_iterator<size_t>{},
              std::back_inserter(this->lengths_));
  });

  lp.register_parse_closure(
      "minibatch_sizes", [this](auto &iss) { iss >> this->minibatch_sizes_; });
  return lp;
}

void IndicesInfo::store_to_file(const std::filesystem::path &path) const {
  std::ofstream out(path);
  out.exceptions(std::ofstream::failbit);
  if (!out) {
    static const thread_local std::string error_message =
        fmt::format("Unable to open table's info file for write. Reason: {}",
                    strerror(errno));
    throw std::runtime_error(error_message.c_str());
  }

  fmt::print(out, "lengths = {}\n", fmt::join(lengths(), " "));
  fmt::print(out, "minibatch_sizes = {}\n", minibatch_sizes_);
}
