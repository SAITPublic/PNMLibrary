#include "common.h"
#include "multiprocess.h"
#include "multithreaded.h"

#include "common/compiler_internal.h"
#include "common/topology_constants.h"

#include "test/mocks/context/utils.h"
#include "test/sls_app/api/constants.h"
#include "test/sls_app/api/structs.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-param-util.h>
#include <gtest/internal/gtest-port.h>

#include "CLI/App.hpp"
#include "CLI/CLI.hpp" // NOLINT(misc-include-cleaner)
#include "CLI/Validators.hpp"

#include <fmt/core.h>

#include <linux/sls_resources.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
using namespace test_app;

namespace {

int user_num_engines; // get from cmd line
/* Unfortunately there is dependence between num_table and mini_batch_size
 * what we can process at the moment, but logically it's different entities
 * SlsRunParams and SlsModelParams, so use pair for this special case
 */

const std::vector test_sparse_feature_sizes =
    !TSAN && PNM_PLATFORM != HARDWARE ? std::vector{16, 32} : std::vector{16};

const std::vector<
    std::pair<size_t /*min_num_lookup*/, size_t /*max_num_lookup*/>>
    test_min_and_max_num_lookup =
        !TSAN ? std::vector<std::pair<size_t, size_t>>{std::pair{1, 500},
                                                       std::pair{40, 40}}
              : std::vector<std::pair<size_t, size_t>>{std::pair{40, 40}};

const auto test_user_preferences =
    PNM_PLATFORM != HARDWARE
        ? std::vector{SLS_ALLOC_AUTO,
                      /* SLS_ALLOC_REPLICATE_ALL == SLS_ALLOC_AUTO, */
                      SLS_ALLOC_DISTRIBUTE_ALL}
        : std::vector{SLS_ALLOC_AUTO};

constexpr std::array<std::pair<int /*mini_batch_size*/, int /*num_table*/>, 2>
    test_batch_sizes_num_tables{{{16, 50}, {128, 6}}};

constexpr std::array test_num_idx_values{32};

constexpr std::array test_context_dummy{test::mock::ContextType::SLS};
constexpr std::array test_context{
    test::mock::ContextType::SLS,
    test::mock::ContextType::RANDOM_OFFSETS_ALLOCATOR};

class SlsTests
    : public testing::TestWithParam<std::tuple<
          std::pair<int /*mini_batch_size*/, int /*num_table*/>,
          std::pair<size_t /*max_num_lookup*/, size_t /*min_num_lookup*/>,
          int /*sparse_feature_size*/, int /*num_idx_values */,
          sls_user_preferences /*preference*/, test::mock::ContextType>> {};

class SlsTestsMP : public SlsTests {};
class SlsTestsMT : public SlsTests {};

const auto print_test_name =
    [](const testing::TestParamInfo<SlsTests::ParamType> &info) {
      const int mini_batch_size = std::get<0>(info.param).first;
      const int num_table = std::get<0>(info.param).second;
      const auto [min_num_lookup, max_num_lookup] = std::get<1>(info.param);
      const int sparse_feature_size = std::get<2>(info.param);
      const int num_idx_values = std::get<3>(info.param);
      const sls_user_preferences preference = std::get<4>(info.param);
      const test::mock::ContextType context_type = std::get<5>(info.param);

      // alphanumeric or "_" only, Gtest limit
      return fmt::format(
          "mini_batch_size_{}__sparse_feature_size_{}__num_table_{}"
          "__num_idx_values_{}__min_num_lookup_{}__max_num_lookup_{}"
          "__preference_{}_context_{}",
          mini_batch_size, sparse_feature_size, num_table, num_idx_values,
          min_num_lookup, max_num_lookup, preference, context_type);
    };

template <typename TestClass, typename ParamsGenerator, typename T>
void RunTest(std::tuple<std::pair<int, int>, std::pair<size_t, size_t>, int,
                        int, sls_user_preferences, test::mock::ContextType>
                 params) {
  if constexpr (PNM_PLATFORM == HARDWARE && std::is_same_v<T, uint32_t>) {
    return;
  }
  std::vector<uint8_t> tables;
  auto [batch_size_num_table, min_and_max_num_lookup, sparse_feature_size,
        num_idx_values, preference, context_type] = params;
  auto [mini_batch_size, num_table] = batch_size_num_table;
  auto [min_num_lookup, max_num_lookup] = min_and_max_num_lookup;
  const StandardRunParams batch(
      HelperRunParams(mini_batch_size, knum_requests, context_type),
      TestAppRunParams(min_num_lookup, max_num_lookup, preference, "random",
                       OverflowType::withoutOverflow));

  const SlsModelParams model(sparse_feature_size, num_table, kemb_table_len);
  const SlsTableFillParams<T> fill(num_idx_values);
  ParamsGenerator params_generator;
  std::filesystem::path root;

  const auto runner = TestClass(batch, model, params_generator);
  const auto filler = PeriodicTablesFiller(model, fill);

  filler.print();
  runner.print();

  filler.fill_tables(tables, root);
  auto sls_op_params = runner.generate_sls_run_params(root);

  runner.run(user_num_engines, tables, sls_op_params, root);
}

TEST(SlsTestsMP, PsumBufferOverflow) {
  // Set batch size for guaranteed overflow of psum buffer for each table batch
  const uint32_t mini_batch_size =
      (pnm::sls::device::topo().PSumBufSize /
       (test_sparse_feature_sizes[0] * sizeof(float))) +
      1;
  const uint32_t num_tables = 5;
  const auto sparse_feature_size = test_sparse_feature_sizes[0];
  const auto num_idx_values = test_num_idx_values[0];
  const auto preference = test_user_preferences[0];

  const StandardRunParams batch(
      HelperRunParams(mini_batch_size, knum_requests),
      TestAppRunParams(5, 20, preference, "random",
                       OverflowType::withoutOverflow));
  const SlsModelParams model(sparse_feature_size, num_tables, kemb_table_len);
  const SlsTableFillParams<float> fill(num_idx_values);
  // Set small number of lookups to reduce sls execution time
  // Increasing number of lookups for huge batch size will severely slow sls
  // execution
  const SlsParamsGenerator params_generator;

  const SlsTestMultiprocess<float> test(batch, model, params_generator);
  test.print();

  const PeriodicTablesFiller filler(model, fill);
  std::vector<uint8_t> tables;
  std::filesystem::path root;
  filler.fill_tables(tables, root);
  auto sls_op_params = test.generate_sls_run_params(root);
  test.run(user_num_engines, tables, sls_op_params, root);
}

TEST(SlsTestsMP, InstructionBufferOverflow) {
  const uint32_t mini_batch_size = test_batch_sizes_num_tables[0].first;
  const uint32_t num_tables = 5;
  const auto sparse_feature_size = test_sparse_feature_sizes[0];
  const auto num_idx_values = test_num_idx_values[0];
  const auto preference = test_user_preferences[0];

  const StandardRunParams batch(
      HelperRunParams(mini_batch_size, knum_requests),
      TestAppRunParams(2049, 2149, preference, "random",
                       OverflowType::withoutOverflow));
  ;

  const SlsModelParams model(sparse_feature_size, num_tables, kemb_table_len);
  const SlsTableFillParams<float> fill(num_idx_values);

  const SlsParamsGenerator params_generator;

  const SlsTestMultiprocess<float> test(batch, model, params_generator);
  test.print();

  const PeriodicTablesFiller filler(model, fill);
  std::vector<uint8_t> tables;
  std::filesystem::path root;
  filler.fill_tables(tables, root);
  auto sls_op_params = test.generate_sls_run_params(root);
  test.run(user_num_engines, tables, sls_op_params, root);
}

TEST(SlsTestsMP, InstructionBufferOverflowEach3rdTable) {
  if constexpr (TSAN) {
    return;
  }

  const auto batch_size_num_tables = test_batch_sizes_num_tables[0];
  const auto sparse_feature_size = test_sparse_feature_sizes[0];
  const auto num_idx_values = test_num_idx_values[0];
  const auto preference = test_user_preferences[0];

  const auto [mini_batch_size, num_table] = batch_size_num_tables;

  const StandardRunParams batch(
      HelperRunParams(mini_batch_size, knum_requests),
      TestAppRunParams(1, 100, preference, "random",
                       OverflowType::overflowInEach3rdTable));

  const SlsModelParams model(sparse_feature_size, num_table, kemb_table_len);
  const SlsTableFillParams<float> fill(num_idx_values);

  const SlsParamsGenerator params_generator;

  const SlsTestMultiprocess<float> test(batch, model, params_generator);
  test.print();

  const PeriodicTablesFiller filler(model, fill);
  std::vector<uint8_t> tables;
  std::filesystem::path root;
  filler.fill_tables(tables, root);
  auto sls_op_params = test.generate_sls_run_params(root);
  test.run(user_num_engines, tables, sls_op_params, root);
}

TEST(SlsTestsMP, InstructionBufferOverflowOneTableWithHugeLookups) {
  const auto batch_size_num_tables = test_batch_sizes_num_tables[0];
  const auto sparse_feature_size = test_sparse_feature_sizes[0];
  const auto num_idx_values = test_num_idx_values[0];
  const auto preference = test_user_preferences[0];

  const auto [mini_batch_size, num_tables] = batch_size_num_tables;
  const StandardRunParams batch(
      HelperRunParams(mini_batch_size, knum_requests),
      TestAppRunParams(1, 100, preference, "random",
                       OverflowType::overflowInOneRandomTable));
  const SlsModelParams model(sparse_feature_size, num_tables, kemb_table_len);
  const SlsTableFillParams<float> fill(num_idx_values);

  const SlsParamsGenerator params_generator;

  const SlsTestMultiprocess<float> test(batch, model, params_generator);
  test.print();

  const PeriodicTablesFiller filler(model, fill);
  std::vector<uint8_t> tables;
  std::filesystem::path root;
  filler.fill_tables(tables, root);
  auto sls_op_params = test.generate_sls_run_params(root);
  test.run(user_num_engines, tables, sls_op_params, root);
}

TEST_P(SlsTestsMP, MultiProcessF) {
  RunTest<SlsTestMultiprocess<float>, SlsParamsGenerator, float>(GetParam());
}

TEST_P(SlsTestsMT, MultiThreadedF) {
  RunTest<SlsTestMultithreaded<float>, SlsParamsGenerator, float>(GetParam());
}

TEST_P(SlsTestsMP, MultiProcessI) {
  RunTest<SlsTestMultiprocess<uint32_t>, SlsParamsGenerator, uint32_t>(
      GetParam());
}
TEST_P(SlsTestsMT, MultiThreadedI) {
  RunTest<SlsTestMultithreaded<uint32_t>, SlsParamsGenerator, uint32_t>(
      GetParam());
}

INSTANTIATE_TEST_SUITE_P(
    TestApp, SlsTestsMP,
    /* Generation of all intersections: each as a separate test */
    testing::Combine(testing::ValuesIn(test_batch_sizes_num_tables),
                     testing::ValuesIn(test_min_and_max_num_lookup),
                     testing::ValuesIn(test_sparse_feature_sizes),
                     testing::ValuesIn(test_num_idx_values),
                     testing::ValuesIn(test_user_preferences),
                     testing::ValuesIn(test_context_dummy)),
    print_test_name);

INSTANTIATE_TEST_SUITE_P(
    TestApp, SlsTestsMT,
    /* Generation of all intersections: each as a separate test */
    testing::Combine(testing::ValuesIn(test_batch_sizes_num_tables),
                     testing::ValuesIn(test_min_and_max_num_lookup),
                     testing::ValuesIn(test_sparse_feature_sizes),
                     testing::ValuesIn(test_num_idx_values),
                     testing::ValuesIn(test_user_preferences),
                     testing::ValuesIn(test_context)),
    print_test_name);

} // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv); // removes all recognized Gtest flags

  for (const std::string &arg : testing::internal::GetArgvs()) {
    if (arg == "--gtest_list_tests") {
      // Gtest dummy test run by CMake to dynamically define tests at runtime
      return RUN_ALL_TESTS();
    }
  }

  CLI::App app{"SLS operation test application"};

  app.add_option("num_engines", user_num_engines,
                 "Number of execution engines to start up")
      ->required()
      ->check(CLI::Range(1, kmax_engines));

  CLI11_PARSE(app, argc, argv);

  return RUN_ALL_TESTS();
}
