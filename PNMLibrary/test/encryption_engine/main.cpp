#include "secure/encryption/sls_engine.h"
#include "secure/encryption/sls_mac.h"

#include "common/compiler_internal.h"
#include "common/sls_types.h"

#include "pnmlib/common/128bit_math.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <type_traits>
#include <vector>

/*! \brief Generate test data. */
struct DataGenerator {
  constexpr static auto rows_count = 49;
  constexpr static auto cols_count = 16;

  constexpr static auto search_requests = 15;

  static auto create_table() {
    std::vector<std::vector<uint32_t>> table(
        rows_count, std::vector<uint32_t>(cols_count, 0));

    for (auto i = 0; i < rows_count; ++i) {
      for (auto j = 0; j < cols_count; ++j) {
        table[i][j] = (i << 8) + j;
      }
    }

    return table;
  }

  static auto create_indices() {
    std::vector<uint32_t> indices(search_requests);

    for (size_t k = 0; k < search_requests; ++k) {
      indices[k] = k;
    }

    return indices;
  }
};

auto plus = [](const auto &x, const auto &y)
                NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW { return x + y; };

template <typename C> void dump(const C &range) {
  fmt::print("{}\n", fmt::join(range, " "));
}

template <typename FwIt>
std::vector<uint32_t> table_sls(const std::vector<std::vector<uint32_t>> &data,
                                FwIt index_begin, FwIt index_end) {
  auto sum = [&data](auto sum, auto idx) {
    std::transform(data[idx].begin(), data[idx].end(), sum.begin(), sum.begin(),
                   plus);
    return sum;
  };

  auto cols = data.front().size();
  return std::accumulate(index_begin, index_end,
                         std::vector<uint32_t>(cols, 0U), sum);
}

template <typename T, typename FwIt>
auto tag_sls(const std::vector<T> &data, FwIt index_begin, FwIt index_end) {
  auto sum =
      [&data](const auto &sum, auto i)
          NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW { return sum + data[i]; };
  return std::accumulate(index_begin, index_end, T{}, sum);
}

struct SLS_OP {
  template <typename T1, typename T2>
  constexpr auto operator()(T1 sum, const T2 &value) const {
    std::transform(value.begin(), value.end(), sum.begin(), sum.begin(), plus);
    return sum;
  }
};

TEST(VerificationEngine, TypeCheck) {
  ASSERT_FALSE(std::is_class_v<pnm::uint128_t>);
  ASSERT_EQ(typeid(pnm::uint128_t).hash_code(),
            typeid(unsigned __int128).hash_code());
}

TEST(EncryptionEngine, SLS_No_MAC) {
  const char tkey[] = "123456789012345";
  sls::FixedVector<uint8_t, 16> key{};
  std::copy(tkey, tkey + sizeof(tkey), key.data());
  const sls::secure::EncryptionEngine engine{42, key};

  auto table = DataGenerator::create_table();
  auto indices = DataGenerator::create_indices();
  auto golden = table_sls(table, indices.begin(), indices.end());

  auto encr_tables = table;

  std::vector<uintptr_t> offsets{0};
  offsets.reserve(table.size());
  for (auto &row : encr_tables) {
    auto [it, ebytes] =
        engine.encrypt(row.begin(), row.end(), row.begin(), offsets.back());
    offsets.emplace_back(offsets.back() + ebytes);
  }

  auto psum_encrypted = table_sls(encr_tables, indices.begin(), indices.end());

  std::vector<uint32_t> psum(psum_encrypted.size(), 0);
  std::vector<uintptr_t> selected_offsets(indices.size());
  std::transform(indices.begin(), indices.end(), selected_offsets.begin(),
                 [&offsets](auto idx) { return offsets[idx]; });

  auto offset_view = pnm::make_view(selected_offsets);
  engine.decrypt_psum(psum_encrypted.begin(), psum_encrypted.end(),
                      offset_view.begin(), offset_view.end(), psum.begin(),
                      SLS_OP{});

  dump(golden);
  dump(psum);

  ASSERT_EQ(golden, psum);
}

TEST(EncryptionEngine, SLS_No_MAC_staged) {
  const char tkey[] = "123456789012345";
  sls::FixedVector<uint8_t, 16> key{};
  std::copy(tkey, tkey + sizeof(tkey), key.data());
  const sls::secure::EncryptionEngine engine{42, key};

  auto table = DataGenerator::create_table();
  auto indices = DataGenerator::create_indices();
  auto golden = table_sls(table, indices.begin(), indices.end());

  auto encr_tables = table;

  std::vector<uintptr_t> offsets{0};
  offsets.reserve(table.size());
  for (auto &row : encr_tables) {
    auto [it, ebytes] =
        engine.encrypt(row.begin(), row.end(), row.begin(), offsets.back());
    offsets.emplace_back(offsets.back() + ebytes);
  }

  auto psum_encrypted = table_sls(encr_tables, indices.begin(), indices.end());

  std::vector<uint32_t> psum(psum_encrypted.size(), 0);
  std::vector<uintptr_t> selected_offsets(indices.size());
  std::transform(indices.begin(), indices.end(), selected_offsets.begin(),
                 [&offsets](auto idx) { return offsets[idx]; });

  auto offset_view = pnm::make_view(selected_offsets);
  auto otp_sum = engine.offset_transform_reduce_vec(
      offset_view.begin(), offset_view.end(),
      std::vector<uint32_t>(DataGenerator::cols_count, 0), SLS_OP{});

  engine.decrypt_psum(psum_encrypted.begin(), psum_encrypted.end(),
                      otp_sum.begin(), psum.begin());

  dump(golden);
  dump(psum);

  ASSERT_EQ(golden, psum);
}

void encryption_test_with_mac(bool corrupted_data) {
  const char tkey[] = "123456789012345";
  sls::FixedVector<uint8_t, 16> key{};
  std::copy(tkey, tkey + sizeof(tkey), key.data());
  const sls::secure::EncryptionEngine engine{42, key};

  auto table = DataGenerator::create_table();
  auto indices = DataGenerator::create_indices();
  auto golden = table_sls(table, indices.begin(), indices.end());

  auto encr_tables = table;

  using tag_type = sls::secure::VerificationEngine<>::tag_type;
  const sls::secure::VerificationEngine vengine(42, 315, key);

  std::vector<uintptr_t> offsets{0};
  offsets.reserve(table.size());

  std::vector<tag_type> verification_tag;

  verification_tag.reserve(table.size());
  for (auto &row : encr_tables) {
    verification_tag.emplace_back(
        vengine.generate_mac(row.begin(), row.end(), offsets.back()));

    auto [it, ebytes] =
        engine.encrypt(row.begin(), row.end(), row.begin(), offsets.back());

    offsets.emplace_back(offsets.back() + ebytes);
  }

  if (corrupted_data) {
    encr_tables.front()[14] = 56;
  }

  auto psum_encrypted = table_sls(encr_tables, indices.begin(), indices.end());
  auto encrypted_tag =
      tag_sls(verification_tag, indices.begin(), indices.end());

  std::vector<uint32_t> psum(psum_encrypted.size(), 0);
  std::vector<uintptr_t> selected_offsets(indices.size());
  std::transform(indices.begin(), indices.end(), selected_offsets.begin(),
                 [&offsets](auto idx) { return offsets[idx]; });

  auto offset_view = pnm::make_view(selected_offsets);
  engine.decrypt_psum(psum_encrypted.begin(), psum_encrypted.end(),
                      offset_view.begin(), offset_view.end(), psum.begin(),
                      SLS_OP{});

  const bool verified =
      vengine.validate_psum(psum.begin(), psum.end(), offset_view.begin(),
                            offset_view.end(), encrypted_tag, plus);

  // Alternative approach for verification
  auto tag_sum = vengine.offset_transform_reduce_vec(
      offset_view.begin(), offset_view.end(), tag_type{}, plus);
  auto verified_alt =
      vengine.validate_psum(psum.begin(), psum.end(), encrypted_tag, tag_sum);
  ASSERT_EQ(verified, verified_alt);
  //======================================================================

  if (corrupted_data) {
    ASSERT_TRUE(!verified);
    ASSERT_NE(golden, psum);
  } else {
    ASSERT_TRUE(verified);
    ASSERT_EQ(golden, psum);
  }

  dump(golden);
  dump(psum);
}

TEST(VerificationEngine, SLS_Normal_data) { encryption_test_with_mac(false); }

TEST(VerificationEngine, SLS_Corrupted_data) { encryption_test_with_mac(true); }
