#include "helper.h"

#include "common/topology_constants.h"

#include "test/mocks/context/utils.h"
#include "test/sls_app/api/constants.h"
#include "test/sls_app/api/structs.h"
#include "test/sls_app/api/utils.h"

#include "pnmlib/core/device.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <random>
#include <stdexcept>
#include <vector>

using namespace test_app;
using pnm::sls::device::topo;

namespace {
template <typename T>
size_t gen_num_tables(const pnm::DevicePointer device, bool distribute_policy) {
  /* Get BASE  memory size*/
  const auto sls_memory_size = device->memory_size();

  fmt::print("SLS total memory size[GB]: {}\n", sls_memory_size >> 30);

  /* 2MB CXL DAX alignment*/
  static constexpr size_t align_size =
      2 << 20; //[TODO: @s-motov]: export alignment as parameter

  /* Calculate table size */
  const auto table_size = sizeof(T) * ksparse_feature_size * kemb_table_len;

  /* Calculate table size with align gap */
  const auto table_size_aligned =
      ((table_size + align_size - 1) / align_size) * align_size;

  size_t num_tables = sls_memory_size / table_size_aligned;

  /* to avoid uneven distribution */
  num_tables -= num_tables % topo().NumOfCUnits;

  return distribute_policy ? num_tables : num_tables / topo().NumOfCUnits;
}

template <typename T>
void gen_tables(std::vector<uint8_t> &tables, size_t num_tables) {
  fmt::print("Generate tables...\n");
  const uint64_t total_tables_features = kemb_table_len * num_tables;
  const size_t tables_size_in_bytes =
      total_tables_features * ksparse_feature_size * sizeof(T);
  fmt::print("Tables size[GB]: {}\n", tables_size_in_bytes >> 30);
  const auto max_reserve_memory = get_mem_avail_in_bytes();
  if (tables_size_in_bytes > max_reserve_memory) {
    fmt::print(stderr, "Tables total size is too big: {}\n",
               tables_size_in_bytes);
    fmt::print(stderr, "Available RAM size: {}\n", max_reserve_memory);
    throw std::runtime_error("Can't generate tables");
  }

  tables.clear();
  tables.resize(tables_size_in_bytes);

  auto *typed_tables = reinterpret_cast<T *>(tables.data());

  for (size_t feature_id = 0; feature_id < total_tables_features;
       ++feature_id) {
    const T sparse_feature_part =
        (feature_id % knum_idx_value + 1) * kseed_sparse_feature_val<T>;
    const uint32_t ft_size = ksparse_feature_size;
    std::fill_n(typed_tables + ft_size * feature_id, ft_size,
                sparse_feature_part);
  }
  fmt::print("Tables generated\n");
}

std::vector<uint32_t>
generate_sparse_indices_vec(const std::vector<uint32_t> &lS_lengths,
                            size_t num_tables) {
  const int indices_size = kmini_batch_size * num_tables;
  // Max number of sparse vectors of all tables of all batches
  std::vector<uint32_t> sparse_indices;
  // Reserve at least part of data, since lookups per batch are random
  sparse_indices.reserve(indices_size);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(0, kemb_table_len - 1);
  const auto random_generator = [&distrib, &gen]() { return distrib(gen); };

  for (size_t table_idx = 0; table_idx < num_tables; ++table_idx) {
    const auto table_batch_offset = table_idx * kmini_batch_size;
    for (auto batch_idx = 0; batch_idx < kmini_batch_size; ++batch_idx) {
      std::generate_n(std::back_inserter(sparse_indices),
                      lS_lengths[table_batch_offset + batch_idx],
                      random_generator);
    }
  }
  return sparse_indices;
}
} // namespace

TEST(Memory, ReplicateHugeTables) {
  static constexpr auto distribute = false;

  const size_t num_tables = gen_num_tables<float>(
      pnm::Device::make_device(pnm::Device::Type::SLS), distribute);

  const HelperRunParams batch(kmini_batch_size, knum_requests,
                              test::mock::ContextType::SLS);
  const SlsModelParams model(ksparse_feature_size, num_tables, kemb_table_len);

  const SlsParamsGenerator params_generator;

  const SlsTestHugeTables<float> test(batch, model, params_generator);
  test.print();

  std::vector<uint8_t> tables;
  gen_tables<float>(tables, num_tables);

  SlsOpParams sls_op_params;
  sls_op_params.lS_lengths = std::vector<uint32_t>(
      num_tables * kmini_batch_size, knum_indices_per_lookup_fix);
  sls_op_params.lS_indices =
      generate_sparse_indices_vec(sls_op_params.lS_lengths, num_tables);
  test.run(tables, sls_op_params, distribute);
}

TEST(Memory, DistributeHugeTables) {
  static constexpr auto distribute = true;

  if (topo().Bus == pnm::sls::device::BusType::AXDIMM &&
      topo().NumOfCUnits > 4) {
    GTEST_SKIP() << "Requires more memory then our CI has\n";
  }

  const size_t num_tables = gen_num_tables<float>(
      pnm::Device::make_device(pnm::Device::Type::SLS), distribute);

  const HelperRunParams batch(kmini_batch_size, knum_requests,
                              test::mock::ContextType::SLS);
  const SlsModelParams model(ksparse_feature_size, num_tables, kemb_table_len);

  const SlsParamsGenerator params_generator;

  const SlsTestHugeTables<float> test(batch, model, params_generator);
  test.print();

  std::vector<uint8_t> tables;
  gen_tables<float>(tables, num_tables);

  SlsOpParams sls_op_params;
  sls_op_params.lS_lengths = std::vector<uint32_t>(
      num_tables * kmini_batch_size, knum_indices_per_lookup_fix);
  sls_op_params.lS_indices =
      generate_sparse_indices_vec(sls_op_params.lS_lengths, num_tables);
  test.run(tables, sls_op_params, distribute);
}
