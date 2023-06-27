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

#ifndef IMDB_INDEX_VECTORS_IO_H
#define IMDB_INDEX_VECTORS_IO_H

#include "vector_of_vectors_info.h"

#include "common/mapped_file.h"

#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/common/misc_utils.h"
#include "pnmlib/common/views.h"

#include <fmt/core.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace tools::gen::imdb {

using namespace pnm::imdb;

namespace literals {
inline constexpr auto index_vec_suffix_bin = ".index_vec.bin";
inline constexpr auto index_vec_suffix_info = ".index_vec.info";
} // namespace literals

/** @brief Class for file read/write operations for index vectors. */
class IndexVectorsIO {
public:
  static void store_to_file(const IndexVectors &index_vectors,
                            const std::filesystem::path &root,
                            const std::string &filename_prefix,
                            const VectorOfVectorsInfo &info) {
    const auto data_path =
        root / (filename_prefix + literals::index_vec_suffix_bin);
    const auto info_path =
        root / (filename_prefix + literals::index_vec_suffix_info);
    info.store_to_file(info_path);

    std::ofstream out(data_path, std::ios::out | std::ios::binary);
    out.exceptions(std::ofstream::badbit | std::ostream::failbit);
    if (out) {
      for (const auto &indexvec : index_vectors) {
        out.write(reinterpret_cast<const char *>(indexvec.data()),
                  sizeof(index_type) * indexvec.size());
      }
    } else {
      throw std::runtime_error(fmt::format(
          "Unable to open index vectors file in write mode. Reason: {}",
          strerror(errno)));
    }
  }

  static IndexVectors load_from_file(const VectorOfVectorsMmap &mmapped_vecs) {
    IndexVectors index_vectors;
    index_vectors.reserve(mmapped_vecs.info.sizes().size());

    size_t expected_size = 0;
    for (auto size : mmapped_vecs.info.sizes()) {
      index_vectors.emplace_back(size);
      expected_size += pnm::utils::byte_size_of_container(index_vectors.back());
    }

    if (expected_size != mmapped_vecs.mmapped_file.size()) {
      throw std::runtime_error(fmt::format(
          "Expected index vectors file size ({} bytes) doesn't match "
          "actual size ({} bytes)",
          expected_size, mmapped_vecs.mmapped_file.size()));
    }
    const auto view = mmapped_vecs.mmapped_file.get_view<index_type>();
    const auto *begin_it = view.begin();
    for (auto &indexvec : index_vectors) {
      const auto *const end_it = begin_it + indexvec.size();
      std::copy(begin_it, end_it, indexvec.begin());
      begin_it = end_it;
    }

    return index_vectors;
  }
};
} // namespace tools::gen::imdb

#endif // IMDB_INDEX_VECTORS_IO_H
