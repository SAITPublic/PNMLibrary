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

#ifndef IMDB_BIT_VECS_IO_H
#define IMDB_BIT_VECS_IO_H

#include "vector_of_vectors_info.h"

#include "common/mapped_file.h"

#include "pnmlib/imdb/bit_containers.h"
#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/common/views.h"

#include <fmt/core.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>

namespace tools::gen::imdb {

using namespace pnm::imdb;

namespace literals {
inline constexpr auto bit_vec_suffix_bin = ".bit_vec.bin";
inline constexpr auto bit_vec_suffix_info = ".bit_vec.info";
} // namespace literals

/** @brief Class for file read/write operations for bit vectors. */
class BitVectorsIO {
public:
  static void store_to_file(const BitVectors &bit_vectors,
                            const std::filesystem::path &root,
                            const std::string &filename_prefix,
                            const VectorOfVectorsInfo &bitvecs_info) {
    const auto data_path =
        root / (filename_prefix + literals::bit_vec_suffix_bin);
    const auto info_path =
        root / (filename_prefix + literals::bit_vec_suffix_info);
    bitvecs_info.store_to_file(info_path);

    std::ofstream out(data_path, std::ios::out | std::ios::binary);
    out.exceptions(std::ofstream::badbit | std::ofstream::failbit);
    if (out) {
      for (const auto &bitvec : bit_vectors) {
        out.write(reinterpret_cast<const char *>(bitvec.container().data()),
                  bitvec.bytes());
      }
    } else {
      throw std::runtime_error(fmt::format(
          "Unable to open bit vector file in write mode. Reason: {}",
          strerror(errno)));
    }
  }

  // dublicates PredicateVectorsIO::load_from_file mostly
  static BitVectors load_from_file(const VectorOfVectorsMmap &mmapped_vecs) {
    BitVectors bit_vectors;
    bit_vectors.reserve(mmapped_vecs.info.sizes().size());

    size_t expected_size = 0;
    for (auto size : mmapped_vecs.info.sizes()) {
      bit_vectors.emplace_back(size);
      expected_size += bit_vectors.back().bytes();
    }

    if (expected_size != mmapped_vecs.mmapped_file.size()) {
      throw std::runtime_error(
          fmt::format("Expected bit vectors file size ({} bytes) doesn't match "
                      "actual size ({} bytes)",
                      expected_size, mmapped_vecs.mmapped_file.size()));
    }
    const auto view = mmapped_vecs.mmapped_file.get_view<index_type>();
    const auto *begin_it = view.begin();
    for (auto &bitvec : bit_vectors) {
      auto &container = bitvec.container();
      const auto *const end_it = begin_it + container.size();
      std::copy(begin_it, end_it, container.begin());
      begin_it = end_it;
    }

    return bit_vectors;
  }
};

} // namespace tools::gen::imdb

#endif // IMDB_BIT_VECS_IO_H
