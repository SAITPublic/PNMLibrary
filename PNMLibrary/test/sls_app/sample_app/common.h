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

#ifndef _SLS_TEST_COMMON_H
#define _SLS_TEST_COMMON_H

#include "test/sls_app/api/constants.h"
#include "test/sls_app/api/structs.h"
#include "test/sls_app/api/utils.h"
#include "tools/datagen/sls/utils.h"

#include <fmt/core.h>

#include <linux/sls_resources.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace test_app {

enum class OverflowType {
  withoutOverflow,
  overflowInEach3rdTable,
  overflowInOneRandomTable
};

inline std::string overflow_type_to_string(OverflowType overflow_type) {
  switch (overflow_type) {
  case (OverflowType::overflowInEach3rdTable):
    return "overflowInEach3rdTable";
  case (OverflowType::overflowInOneRandomTable):
    return "overflowInOneRandomTable";
  case (OverflowType::withoutOverflow):
    return "withoutOverflow";
  default:
    return "overflowUndefined";
  }
}

// This structure provides test_app specific runtime parameters for SLS
struct TestAppRunParams {
  TestAppRunParams(const size_t min_num_lookup, const size_t max_num_lookup,
                   sls_user_preferences pref,
                   const std::string &lengths_generation_type,
                   OverflowType overflow_type)
      : min_num_lookup(min_num_lookup), max_num_lookup(max_num_lookup),
        preference(pref), lengths_generation_type(lengths_generation_type),
        overflow_type(overflow_type) {}

  void print() const {
    fmt::print("TestAppRunParams:\n");
    fmt::print("\tmin_num_lookup: {}\n", min_num_lookup);
    fmt::print("\tmax_num_lookup: {}\n", max_num_lookup);
    fmt::print("\tpreference: {}\n", preference_to_str(preference));
    fmt::print("\tlengths_generation_type: {}\n", lengths_generation_type);
    fmt::print("\toverflow_type: {}\n", overflow_type_to_string(overflow_type));
  }

  const size_t min_num_lookup;
  const size_t max_num_lookup;
  const sls_user_preferences preference;
  const std::string lengths_generation_type;
  const OverflowType overflow_type;
};

struct StandardRunParams {
  StandardRunParams(const HelperRunParams &helper_params,
                    const TestAppRunParams &testapp_params)
      : helper_params(helper_params), testapp_params(testapp_params) {}

  void print() const {
    helper_params.print();
    testapp_params.print();
  }

  const HelperRunParams helper_params;
  const TestAppRunParams testapp_params;
};

inline std::string
generate_indices_path(const HelperRunParams &helper_run_params,
                      const TestAppRunParams &testapp_run_params) {
  return fmt::format(
      "Ml_{}_ml_{}_bs_{}_{}_{}", testapp_run_params.max_num_lookup,
      testapp_run_params.min_num_lookup, helper_run_params.mini_batch_size,
      testapp_run_params.lengths_generation_type,
      overflow_type_to_string(testapp_run_params.overflow_type));
}

template <typename T>
inline std::filesystem::path
generate_tables_path(const SlsModelParams &model_params,
                     const SlsTableFillParams<T> &fill_params) {
  const auto table_name =
      fmt::format("test_app_{}_n_{}_Ms_{}_ms_{}_sparse_{}_idx_{}",
                  fill_params.value_type, model_params.num_tables(),
                  model_params.emb_table_len(), model_params.emb_table_len(),
                  model_params.sparse_feature_size, fill_params.num_idx_values);

  std::filesystem::path root_tables = PathToTables::value / table_name;

  return root_tables;
}

template <typename T> class PeriodicTablesFiller {
public:
  PeriodicTablesFiller(const SlsModelParams &model_params,
                       const SlsTableFillParams<T> &fill_params)
      : model_params_(model_params), fill_params_(fill_params) {}

  void print() const { fill_params_.print(); }

  void fill_tables(std::vector<uint8_t> &tables,
                   std::filesystem::path &root) const {
    fmt::print("Filling tables for new parameters.\n");

    tables.clear();

    const uint64_t total_tables_features =
        model_params_.total_tables_features();
    const auto max_reserve_memory = get_mem_avail_in_bytes();
    const size_t tables_size_bytes =
        total_tables_features * model_params_.sparse_feature_size * sizeof(T);
    if (max_reserve_memory < tables_size_bytes) {
      fmt::print(stderr, "Tables total size is too big: {}\n",
                 tables_size_bytes);
      fmt::print(stderr, "Available RAM size: {}\n", max_reserve_memory);
      throw std::runtime_error("Can't fill tables");
    }

    root = generate_tables_path(model_params_, fill_params_);

    const std::string generator_name = fill_params_.value_type + "_tapp";
    const std::string generator_args =
        std::to_string(fill_params_.num_idx_values) + " " +
        std::to_string(model_params_.sparse_feature_size);

    sls::tests::get_or_create_test_tables(
        root, generator_name, generator_args, model_params_.sparse_feature_size,
        model_params_.num_tables(), model_params_.emb_table_len(), tables);

    fmt::print("Tables filled.\n");
  }

private:
  const SlsModelParams model_params_;
  const SlsTableFillParams<T> fill_params_;
};

} // namespace test_app

#endif // _SLS_TEST_COMMON_H
