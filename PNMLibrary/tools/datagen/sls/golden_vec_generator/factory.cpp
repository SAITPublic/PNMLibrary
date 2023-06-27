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

#include "factory.h"

#include "common/cpu_sls.h"
#include "common/mapped_file.h"

#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/general/test_tables_mmap.h"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

template <typename T>
struct TrivialGoldenVecGenerator : public IGoldenVecGenerator {
private:
  std::vector<uint8_t>
  compute_golden_sls_impl(const void *tables, const TablesInfo &tables_meta,
                          const uint32_t *indices,
                          const IndicesInfo &indices_meta) const override {

    const auto mini_batch_size = indices_meta.minibatch_size();
    const auto num_tables = tables_meta.num_tables();
    const auto sparse_feature_size = tables_meta.cols();

    std::vector<T> golden(mini_batch_size * num_tables * sparse_feature_size);
    auto *const psum = golden.data();

    const auto *data = static_cast<const T *>(tables);
    const auto &rows = tables_meta.rows();
    const auto *lengths = indices_meta.lengths().data();

    SLS<T>::compute(data, rows, indices, lengths, sparse_feature_size,
                    mini_batch_size, psum);

    return {reinterpret_cast<uint8_t *>(golden.data()),
            reinterpret_cast<uint8_t *>(golden.data() + golden.size())};
  }
};

void IGoldenVecGenerator::compute_and_store_golden_sls(
    const std::filesystem::path &root, const std::string &prefix,
    sls::tests::TablesMmap &tables, sls::tests::Indices &indices) const {

  auto file_size = [](std::istream &s) {
    s.seekg(0, std::ios_base::end);
    auto size = s.tellg();
    s.seekg(0, std::ios_base::beg);
    return size;
  };

  auto indices_size = file_size(indices.in);
  std::vector<uint32_t> emb_indices(indices_size / sizeof(uint32_t));

  indices.in.read(reinterpret_cast<char *>(emb_indices.data()), indices_size);

  auto golden = compute_golden_sls_impl(tables.mapped_file.data(), tables.info,
                                        emb_indices.data(), indices.info);

  const auto bin_path = root / (prefix + ".golden.bin");

  std::ofstream out(bin_path, std::ios::out | std::ios::binary);
  out.exceptions(std::ofstream::badbit);
  if (out) {
    out.write(reinterpret_cast<char *>(golden.data()), golden.size());
  } else {
    static const thread_local std::string error_message =
        std::string{"Unable open file in write mode. Reason: "} +
        strerror(errno);
    throw std::runtime_error(error_message.c_str());
  }
}

GoldenVecGeneratorFactory &GoldenVecGeneratorFactory::default_factory() {
  static GoldenVecGeneratorFactory factory;

  factory.register_builder("uint32_t", []() {
    return std::make_unique<TrivialGoldenVecGenerator<uint32_t>>();
  });
  factory.register_builder("uint64_t", []() {
    return std::make_unique<TrivialGoldenVecGenerator<uint64_t>>();
  });
  factory.register_builder("float", []() {
    return std::make_unique<TrivialGoldenVecGenerator<float>>();
  });

  return factory;
}
