/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or
 * computer language in any form
 * by any means, electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung
 * Electronics.
 */

#include "common/profile.h"

#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/indices_generator/base.h"
#include "tools/datagen/sls/indices_generator/factory.h"
#include "tools/datagen/sls/lengths_generator/factory.h"
#include "tools/datagen/sls/tables_generator/factory.h"

#include "pnmlib/sls/embedded_tables.h"
#include "pnmlib/sls/operation.h"
#include "pnmlib/sls/type.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/runner.h"
#include "pnmlib/core/sls_device.h"

#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <stdlib.h> // NOLINT(modernize-deprecated-headers)

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

// Common test parameters:
inline constexpr size_t num_tables = 2;
inline constexpr std::array<uint32_t, 2> sparse_feature_sizes = {16, 32};
inline constexpr int idx_values = 32;
inline constexpr int table_len = 2'000'000;

// For these parameters it is known:
inline constexpr int max_inst_per_table = 16'384;

// Test sizes
inline constexpr std::string_view small_size = "small tests";
inline constexpr std::string_view big_size = "big tests";
inline constexpr std::string_view random_size = "random tests";

class TestParameters {

public:
  void add_parameter(std::string_view test_size, int mini_batch_size,
                     int min_num_lookup, int max_num_lookup) {
    auto parameters_pack =
        std::make_tuple(mini_batch_size, min_num_lookup, max_num_lookup);
    auto &vec = test_batch_sizes_num_lookups_[test_size];
    if ((max_num_lookup < max_inst_per_table) &&
        (std::find(vec.begin(), vec.end(), parameters_pack) == vec.end())) {
      vec.push_back(parameters_pack);
    }
  }

  void generate_test_parameters() {
    // small tests
    for (int mini_batch_size = 8; mini_batch_size <= 512;
         mini_batch_size *= 1.5) {
      for (int inst = 512; inst < max_inst_per_table; inst *= 1.5) {
        add_parameter(small_size, mini_batch_size, inst / mini_batch_size,
                      inst / mini_batch_size);
      }
    }

    // big tests
    for (int mini_batch_size = 8; mini_batch_size <= 512;
         mini_batch_size *= 2) {
      for (int inst = max_inst_per_table; inst < 800'000; inst *= 2) {
        add_parameter(big_size, mini_batch_size, inst / mini_batch_size,
                      inst / mini_batch_size);
      }
    }

    // random tests
    for (int mini_batch_size = 8; mini_batch_size <= 512;
         mini_batch_size *= 2) {
      for (int inst = 512; inst < 800'000; inst *= 4) {
        add_parameter(random_size, mini_batch_size,
                      std::max(1, inst / mini_batch_size / 2),
                      std::max(2, inst * 3 / (mini_batch_size * 2)));

        add_parameter(random_size, mini_batch_size,
                      std::max(1, inst / mini_batch_size - 10),
                      inst / mini_batch_size + 10);
      }
    }
  }

  auto &test_batch_sizes_num_lookups() { return test_batch_sizes_num_lookups_; }

private:
  std::map<
      std::string_view,
      std::vector<std::tuple<int /*mini_batch_size*/, int /*min_num_lookup*/,
                             int /*max_num_lookup*/>>>
      test_batch_sizes_num_lookups_;
};

class TestHelper {
public:
  TestHelper(uint32_t sparse_feature_size)
      : sparse_feature_size(sparse_feature_size) {}

  void create_tables() {
    rows_ = std::vector<uint32_t>(num_tables, table_len);
    tinfo_ = TablesInfo(num_tables, sparse_feature_size, rows_);
    auto table_generator = TablesGeneratorFactory::default_factory().create(
        "float_tapp", fmt::format("{} {}", idx_values, sparse_feature_size));

    tables_ = table_generator->create(tinfo_);
  }

  void create_device() {
    device_ = SlsDevice::make(pnm::Device::Type::SLS_AXDIMM);
  }

  void write_tables_to_device() {
    const auto sparse_feature_size_bytes = sparse_feature_size * sizeof(float);
    emb_tables_ = pnm::memory::EmbeddedTables::create(
        pnm::make_const_view(tables_), rows_, sparse_feature_size_bytes,
        context_);
  }

  void create_indices_generator() {
    indices_generator_ =
        IndicesGeneratorFactory::default_factory().create("random");
  }

  void prepare_tables() {
    create_tables();
    create_device();
    write_tables_to_device();
    create_indices_generator();
  }

  void generate_indices(std::string_view test_type, const int mini_batch_size,
                        const int min_num_lookup, const int max_num_lookup,
                        const int knum_requests) {
    if (test_type == random_size) {
      auto length_generator = LengthsGeneratorFactory::default_factory().create(
          "random", std::to_string(0));

      auto lengths = length_generator->create(
          tinfo_.num_tables(), min_num_lookup, max_num_lookup, mini_batch_size);
      info_ = IndicesInfo(mini_batch_size, lengths);

    } else {
      std::vector<uint32_t> lookups_per_table(num_tables, max_num_lookup);
      info_ = IndicesInfo(std::vector<size_t>(lookups_per_table.begin(),
                                              lookups_per_table.end()),
                          mini_batch_size);
    }

    // Indices generation
    indices_.clear();
    std::generate_n(
        std::back_inserter(indices_), knum_requests,
        [&indices_generator = indices_generator_, &info = info_,
         &tinfo = tinfo_]() { return indices_generator->create(info, tinfo); });
  }

  void create_operation() {
    sls_op_ = PNMSLSOperation{sparse_feature_size, pnm::make_const_view(rows_),
                              emb_tables_.get(), SLSType::Float};
  }

  int count_inst() {
    return std::accumulate(info_.lengths().begin(), info_.lengths().end(), 0);
  }

  void run_sls(int i) {
    std::vector<float> psum = std::vector<float>(
        info_.minibatch_size() * num_tables * sparse_feature_size);
    sls_op_.set_run_params(info_.minibatch_size(),
                           pnm::make_view(info_.lengths()),
                           pnm::make_const_view(indices_[i]),
                           pnm::view_cast<uint8_t>(pnm::make_view(psum)));
    runner_.run(sls_op_);
  }

private:
  pnm::ContextHandler context_ =
      pnm::make_context(pnm::Device::Type::SLS_AXDIMM);
  pnm::Runner runner_{context_};

  uint32_t sparse_feature_size;
  std::vector<uint32_t> rows_;
  TablesInfo tinfo_;
  std::vector<uint8_t> tables_;

  SlsDevice device_;

  pnm::memory::EmbeddedTablesHandler emb_tables_;

  std::unique_ptr<IIndicesGenerator> indices_generator_;
  IndicesInfo info_;
  std::vector<std::vector<uint32_t>> indices_;

  PNMSLSOperation sls_op_;
};

class CSVFormat {
public:
  void print_caption(std::vector<float> &buf_dividers) const {
    fmt::print("test type,test size (in instructions),mini_batch_size, "
               "sparse_feature_size");
    for (auto buf_divider : buf_dividers) {
      if (buf_divider < 1) {
        fmt::print(",SLS time with best buf divider");
      } else {
        fmt::print(",SLS time with buf divider {}", buf_divider);
      }
    }
    fmt::print("\n");
  }

  void print_part_of_row(std::string_view test_size, int inst_cout,
                         int mini_batch_size,
                         uint32_t sparse_feature_size) const {
    fmt::print("{},{},{},{}", test_size, inst_cout, mini_batch_size,
               sparse_feature_size);
  }

  void start_sls_time() { timer_sls_.tick(); }

  void print_sls_time(int knum_requests) {
    timer_sls_.tock();

    fmt::print(",{}", (timer_sls_.duration().count() / knum_requests));
  }

  void print_end_of_line() const { fmt::print("\n"); }

private:
  pnm::profile::TimerUsT timer_sls_;
};

// This benchmark reduces the inst buffer size (by buf_divider)
// and looks at what value the speed of the run SLS time will be the best
void RunTest(std::string_view test_size) {

  CSVFormat output_info;

  // inicialize a vector of multiple buf_dividers
  static constexpr int number_of_buf_divider = 100;
  std::vector<float> buf_dividers(number_of_buf_divider);
  std::iota(buf_dividers.begin(), buf_dividers.end(), 0);

  output_info.print_caption(buf_dividers);

  for (const auto &sparse_feature_size : sparse_feature_sizes) {

    TestHelper helper(sparse_feature_size);
    helper.prepare_tables();

    // variable test sizes
    TestParameters parameters;
    parameters.generate_test_parameters();

    static constexpr int knum_requests = 100;

    // Start testing
    const auto &vec = parameters.test_batch_sizes_num_lookups()[test_size];
    for (auto [mini_batch_size, min_num_lookup, max_num_lookup] : vec) {

      helper.generate_indices(test_size, mini_batch_size, min_num_lookup,
                              max_num_lookup, knum_requests);
      helper.create_operation();

      const auto inst_count = helper.count_inst();

      output_info.print_part_of_row(test_size, inst_count, mini_batch_size,
                                    sparse_feature_size);

      // Run SLS operation
      for (auto buf_divider : buf_dividers) {

        // Only for very small tests we want to greatly reduce buffer ышяу
        if (inst_count > 10000 && buf_divider > 40) {
          break;
        }

        unsetenv("SLS_BUF_DIVIDER");

        if (buf_divider > 0) {
          setenv("SLS_BUF_DIVIDER", std::to_string(buf_divider).c_str(), 1);
        }

        output_info.start_sls_time();

        for (int i = 0; i < knum_requests; ++i) {
          helper.run_sls(i);
        }

        output_info.print_sls_time(knum_requests);
      }
      output_info.print_end_of_line();
    }

    unsetenv("SLS_BUF_DIVIDER");
  }
}

TEST(VariationExecNums, SmallTests) { RunTest(small_size); }

TEST(VariationExecNums, BigTests) { RunTest(big_size); }

TEST(VariationExecNums, RandomTests) { RunTest(random_size); }
