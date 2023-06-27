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
#ifndef IMDB_VECTOR_OF_VECTORS_INFO_H
#define IMDB_VECTOR_OF_VECTORS_INFO_H

#include "common/mapped_file.h"

#include "tools/datagen/line_parser.h"

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <istream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tools::gen::imdb {

/** @brief Basic information about vector of vectors structure.*/
class VectorOfVectorsInfo {
public:
  VectorOfVectorsInfo() = default;

  VectorOfVectorsInfo(std::vector<size_t> sizes) : sizes_{std::move(sizes)} {}

  explicit VectorOfVectorsInfo(const std::filesystem::path &path) {
    std::ifstream in(path);
    in.exceptions(std::istream::badbit);
    if (!in) {
      throw std::runtime_error(
          fmt::format("Unable to open vector of vector's info file. Reason: {}",
                      strerror(errno)));
    }

    auto parser = create_parser();
    parser.parse_file(in);
  }

  template <typename VectorOfVectors>
  static VectorOfVectorsInfo
  create_from_vector(const VectorOfVectors &vec_of_vecs) {
    std::vector<size_t> sizes_vec(vec_of_vecs.size());
    std::transform(vec_of_vecs.begin(), vec_of_vecs.end(), sizes_vec.begin(),
                   [](const auto &vec) { return vec.size(); });
    return VectorOfVectorsInfo(sizes_vec);
  }

  const auto &sizes() const { return sizes_; }

  void store_to_file(const std::filesystem::path &path) const {
    std::ofstream out(path);
    out.exceptions(std::ofstream::badbit | std::ofstream::failbit);
    if (!out) {
      throw std::runtime_error(fmt::format(
          "Unable to open vector of vector's info file for write. Reason: {}",
          strerror(errno)));
    }
    fmt::print(out, "sizes = {}\n", fmt::join(sizes(), " "));
  }

private:
  LineParser create_parser() {
    LineParser lp;
    lp.register_parse_closure("sizes", [this](auto &iss) {
      std::copy(std::istream_iterator<size_t>{iss},
                std::istream_iterator<size_t>{},
                std::back_inserter(this->sizes_));
    });

    return lp;
  }

  // vector of sizes of vectors
  std::vector<size_t> sizes_;
};

/** @brief Class that allows to read vectors of vectors via mmap */
class VectorOfVectorsMmap {
public:
  VectorOfVectorsMmap() = default;

  VectorOfVectorsMmap(const std::filesystem::path &data_path,
                      const std::filesystem::path &info_path)
      : mmapped_file(data_path), info(info_path) {}

  pnm::utils::MappedFile mmapped_file;
  VectorOfVectorsInfo info;
};

} // namespace tools::gen::imdb

#endif // IMDB_VECTOR_OF_VECTORS_INFO_H
