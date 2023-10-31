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

#ifndef _SLS_TEST_STRUCTS_H
#define _SLS_TEST_STRUCTS_H

#include "test/mocks/context/utils.h"
#include "tools/datagen/sls/general/traits.h"

#include <fmt/core.h>

#include <cstddef>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

namespace test_app {

// This structure provides runtime parameters for SLS
struct HelperRunParams {
  HelperRunParams(
      int mini_batch_size, int num_requests,
      test::mock::ContextType context_type = test::mock::ContextType::SLS)
      : mini_batch_size(mini_batch_size), num_requests(num_requests),
        context_type(context_type) {}

  void print() const {
    fmt::print("HelperRunParams:\n");
    fmt::print("\tmini_batch_size: {}\n", mini_batch_size);
    fmt::print("\tnum_requests: {}\n", num_requests);
  }
  const int mini_batch_size;
  const int num_requests;
  const test::mock::ContextType context_type;
};

// This structure provides model-level information
struct SlsModelParams {
  SlsModelParams(int sparse_feature_size, int num_table, int emb_table_len)
      : sparse_feature_size(sparse_feature_size),
        tables_rows_num(num_table, emb_table_len) {}

  void print() const {
    fmt::print("SlsModelParams:\n");
    fmt::print("\tsparse_feature_size: {}\n", sparse_feature_size);
    fmt::print("\tnum_table: {}\n", num_tables());
    fmt::print("\temb_table_len: {}\n", emb_table_len());
  }

  uint64_t total_tables_features() const {
    return std::accumulate(tables_rows_num.begin(), tables_rows_num.end(), 0);
  }

  size_t num_tables() const { return tables_rows_num.size(); }

  size_t emb_table_len() const { return tables_rows_num[0]; }

  template <typename T> auto feature_size_in_bytes() const {
    return sparse_feature_size * sizeof(T);
  }

  // [TODO: @e-kutovoi] Use uint32_t or uint64_t here and in all other places
  int sparse_feature_size;
  std::vector<uint32_t> tables_rows_num;
};

// This structure provides information for generating values to store in
// tables. This can be replaced with strategy object if needed.
template <typename T> struct SlsTableFillParams {
  SlsTableFillParams(int num_idx_values) : num_idx_values(num_idx_values) {
    value_type = tools::gen::sls::DataTypeTraits<T>().name;
  }

  void print() const {
    fmt::print("SlsTableFillParams:\n");
    fmt::print("\tnum_idx_values: {}\n", num_idx_values);
    fmt::print("\tvalue_type: {}\n", value_type);
  }

  std::string value_type;
  const int num_idx_values;
};

// lS_indices: Sparse indices of all tables of all batches. Some basic info on
// sparse indices is here https://caffe2.ai/docs/sparse-operations.html
// lS_lengths: Sparse vector count of all tables of all batches.
struct SlsOpParams {
  std::vector<uint32_t> lS_indices;
  std::vector<uint32_t> lS_lengths;
};

/** @brief Base test parameters generator */
class SlsParamsGenerator {
public:
  virtual ~SlsParamsGenerator() = default;

  template <typename T>
  std::vector<T> generate_psum(const HelperRunParams &run_params,
                               const SlsModelParams &model_params,
                               T init_val) const {
    // psum_res: fill result buffer with 1.0f value for easier error detection.
    return std::vector<T>(run_params.mini_batch_size *
                              model_params.num_tables() *
                              model_params.sparse_feature_size,
                          init_val);
  }
};

} // namespace test_app

#endif // _SLS_TEST_STRUCTS_H
