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

#ifndef IMDB_GOLDEN_VEC_GEN_H
#define IMDB_GOLDEN_VEC_GEN_H

#include "imdb/utils/runner/avx2_scan.h"

#include "tools/datagen/imdb/general/bit_vectors_io.h"
#include "tools/datagen/imdb/general/index_vectors_io.h"
#include "tools/datagen/imdb/general/vector_of_vectors_info.h"

#include "pnmlib/imdb/scan_types.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace tools::gen::imdb {

using namespace pnm::imdb::runner;

/** @brief Golden vector generator for scan operation */
class ImdbGoldenVecGenerator {
public:
  template <typename Output, typename Predicate, typename Runner = AVX2Runner>
  static Output scan(const compressed_vector &column,
                     const Predicate &predicates) {
    Runner runner{column};
    if constexpr (std::is_same_v<Output, IndexVectors> &&
                  std::is_same_v<Predicate, Ranges>) {
      return runner.compute_in_range_to_iv(predicates);
    } else if constexpr (std::is_same_v<Output, IndexVectors> &&
                         std::is_same_v<Predicate, Predictors>) {
      return runner.compute_in_list_to_iv(predicates);
    } else if constexpr (std::is_same_v<Output, BitVectors> &&
                         std::is_same_v<Predicate, Ranges>) {
      return runner.compute_in_range_to_bv(predicates);
    } else if constexpr (std::is_same_v<Output, BitVectors> &&
                         std::is_same_v<Predicate, Predictors>) {
      return runner.compute_in_list_to_bv(predicates);
    } else {
      throw std::runtime_error("Got invalid operation or output_type!");
    }
  }

  template <typename Output, typename Predicate, typename Runner = AVX2Runner>
  static void compute_and_store_golden(const std::filesystem::path &root,
                                       const std::string &filename_prefix,
                                       const compressed_vector &column,
                                       const Predicate &predicates) {
    Output golden = scan<Output, Predicate, Runner>(column, predicates);
    const auto &info = VectorOfVectorsInfo::create_from_vector(golden);
    if constexpr (std::is_same_v<Output, BitVectors>) {
      BitVectorsIO::store_to_file(golden, root, filename_prefix, info);
    } else {
      IndexVectorsIO::store_to_file(golden, root, filename_prefix, info);
    }
  }
};
} // namespace tools::gen::imdb

#endif // IMDB_GOLDEN_VEC_GEN_H
