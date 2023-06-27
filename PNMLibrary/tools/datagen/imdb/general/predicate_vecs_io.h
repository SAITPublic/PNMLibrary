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

#ifndef IMDB_PREDICATE_VECS_IO_H
#define IMDB_PREDICATE_VECS_IO_H

#include "predicate_vecs_info.h"

#include "common/mapped_file.h"

#include "pnmlib/imdb/scan_types.h"

#include <filesystem>
#include <string>

namespace tools::gen::imdb {

using namespace pnm::imdb;

inline constexpr auto predicate_vec_suffix_bin = ".predicate_vec.bin";
inline constexpr auto predicate_vec_suffix_info = ".predicate_vec.info";

/*! \brief Class that allows to read predicate vectors via mmap */
class PredicateVectorsMmap {
public:
  PredicateVectorsMmap() = default;

  PredicateVectorsMmap(const std::filesystem::path &data_path,
                       const std::filesystem::path &info_path)
      : mmapped_file(data_path), info(info_path) {}

  pnm::utils::MappedFile mmapped_file;
  PredicateVectorsInfo info;
};

/*! \brief Class for file read/write operations for predicate vectors. */
class PredicateVectorsIO {
public:
  static void store_to_file(const Predictors &predictors,
                            const std::filesystem::path &root,
                            const std::string &filename_prefix,
                            const PredicateVectorsInfo &predicate_info);

  static Predictors load_from_file(const PredicateVectorsMmap &mmapped_vecs);
};

} // namespace tools::gen::imdb

#endif // IMDB_PREDICATE_VECS_IO_H
