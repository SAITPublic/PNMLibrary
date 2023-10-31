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

#ifndef SLS_TEST_UTILS_H
#define SLS_TEST_UTILS_H

#include "common/log.h"
#include "common/mapped_file.h"

#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/general/test_tables_mmap.h"
#include "tools/datagen/sls/golden_vec_generator/factory.h"
#include "tools/datagen/sls/indices_generator/factory.h"
#include "tools/datagen/sls/lengths_generator/factory.h"
#include "tools/datagen/sls/tables_generator/factory.h"

#include "pnmlib/common/misc_utils.h"
#include "pnmlib/common/views.h"

#include <fmt/core.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace tools::gen::sls {

inline constexpr auto tables_default_name = "embedding.bin";
inline constexpr auto info_default_name = "embedding.info";
inline constexpr auto indices_suffix_bin = ".indices.bin";
inline constexpr auto indices_suffix_info = ".indices.info";
inline constexpr auto golden_suffix_bin = ".golden.bin";

struct GeneratorWithOverflowParams {
  size_t max_num_lookup;
  size_t min_num_lookup;
  int mini_batch_size;
  std::filesystem::path root;
  std::string prefix;
  std::string generator_lengths_name;
  std::string overflow_type;
  std::string generator_indices_name;
  std::string entry_type;
};

inline TablesMmap get_test_tables_mmap(const std::filesystem::path &root) {
  auto tables_path = root / tables_default_name;
  auto info_path = root / info_default_name;
  return TablesMmap(tables_path, info_path);
}

inline void get_or_create_test_tables(const std::filesystem::path &root,
                                      const std::string &generator_name,
                                      const std::string &generator_args,
                                      size_t sparse_feature_size,
                                      size_t num_tables, size_t emb_table_len,
                                      std::vector<uint8_t> &tables) {

  if (std::filesystem::exists(root / tables_default_name)) {
    pnm::log::info("Embedding tables exist, skipping creation");
  } else {
    pnm::log::info("Embedding tables not found, creating");
    if (!std::filesystem::exists(root) &&
        !std::filesystem::create_directory(root)) {
      PNM_LOG_ERROR(
          "Can't create directory to store embedding tables, path = {}.",
          root.string());
    }

    auto table_generator = TablesGeneratorFactory::default_factory().create(
        generator_name, generator_args);

    const TablesInfo info(num_tables, sparse_feature_size,
                          std::vector<uint32_t>(num_tables, emb_table_len));
    table_generator->create_and_store(root, info);
  }

  auto tables_ = get_test_tables_mmap(root);
  auto tables_view = tables_.mapped_file.get_view<uint8_t>();
  tables = std::vector<uint8_t>(tables_view.begin(), tables_view.end());
}

inline Indices get_test_indices(const std::filesystem::path &root,
                                const std::string &prefix) {
  Indices out;

  auto indices_bin = root / (prefix + indices_suffix_bin);
  auto indices_info = root / (prefix + indices_suffix_info);

  out.in.open(indices_bin, std::ios::binary | std::ios::in);
  if (!out.in) {
    static const thread_local std::string error_message =
        std::string{"Unable to open file with indices. Reason: "} +
        strerror(errno);
    throw std::runtime_error(error_message.c_str());
  }

  out.in.exceptions(std::istream::badbit);

  out.info = IndicesInfo(indices_info);
  return out;
}

inline void
get_or_create_test_indices(const GeneratorWithOverflowParams &params,
                           std::vector<uint32_t> &lengths,
                           std::vector<uint32_t> &indices) {

  if (std::filesystem::exists(params.root /
                              (params.prefix + indices_suffix_bin))) {
    pnm::log::info("Indices exist, skipping creation");
  } else {
    pnm::log::info("Indices have not been found, creating");
    auto indices_generator = IndicesGeneratorFactory::default_factory().create(
        params.generator_indices_name);

    auto length_generator = LengthsGeneratorFactory::default_factory().create(
        params.generator_lengths_name, params.overflow_type);

    auto tables = get_test_tables_mmap(params.root);
    const TablesInfo &tinfo = tables.info;

    auto lengths =
        length_generator->create(tinfo.num_tables(), params.min_num_lookup,
                                 params.max_num_lookup, params.mini_batch_size);
    const IndicesInfo info(params.mini_batch_size, lengths);
    indices_generator->create_and_store(params.root, params.prefix, info,
                                        tinfo);

    auto indices = get_test_indices(params.root, params.prefix);

    auto generator =
        GoldenVecGeneratorFactory::default_factory().create(params.entry_type);
    generator->compute_and_store_golden_sls(params.root, params.prefix, tables,
                                            indices);
  }

  auto ihandler = get_test_indices(params.root, params.prefix);

  auto index_count = std::accumulate(ihandler.info.lengths().begin(),
                                     ihandler.info.lengths().end(), 0ULL);

  lengths = ihandler.info.lengths();

  indices.resize(index_count);
  ihandler.in.read(reinterpret_cast<char *>(indices.data()),
                   pnm::utils::byte_size_of_container(indices));
}

inline std::ifstream get_golden_vector(const std::filesystem::path &root,
                                       const std::string &prefix) {
  auto bin = root / (prefix + golden_suffix_bin);

  std::ifstream in;
  in.open(bin, std::ios::binary | std::ios::in);
  if (!in) {
    static const thread_local std::string error_message =
        std::string{"Unable to open file with golden vector. Reason: "} +
        strerror(errno);
    throw std::runtime_error(error_message.c_str());
  }

  in.exceptions(std::istream::badbit);

  return in;
}

} // namespace tools::gen::sls

#endif // SLS_UTILS_H
