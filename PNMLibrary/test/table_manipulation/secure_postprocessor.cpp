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

#include "fixtures.h"
#include "ptr_rw.h"

#include "secure/encryption/sls_dlrm_postprocessor.h"
#include "secure/encryption/sls_dlrm_preprocessor.h"
#include "secure/encryption/sls_postrocessor.h"

#include "common/compiler_internal.h"

#include "pnmlib/common/128bit_math.h"
#include "pnmlib/common/rowwise_view.h"
#include "pnmlib/common/variable_row_view.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

using namespace sls::secure;

struct SecurePostprocessorSimple : public ::testing::Test {
  const uint32_t index[2] = {0, 1};
  const uint32_t lengths[3] = {2U, 1U, 1U};
  uint64_t data[4] = {1ULL, 2ULL, 3ULL, 4ULL};
  uint64_t edata[4] = {1ULL, 2ULL, 3ULL, 4ULL};
  std::vector<uint64_t> psum, dummy;

  std::vector<pnm::uint128_t> tags;
  std::unique_ptr<IPostprocessor<uint64_t>> handler;
  std::vector<uint8_t> out;

  void SetUp() override {
    tags.resize(2);

    auto [enc, ver] = create_encryption_engine<uint64_t>();
    tags[0] = ver.generate_mac(&data[0], &data[2], 0);
    tags[1] = ver.generate_mac(&data[2], &data[4], 16);

    enc.encrypt(&edata[0], &edata[2], &edata[0], 0);
    enc.encrypt(&edata[2], &edata[4], &edata[2], 16);

    handler = std::unique_ptr<IPostprocessor<uint64_t>>(
        new DLRMPostprocessor<uint64_t>(std::move(enc), std::move(ver), {0},
                                        2));

    psum.resize(8, 0);
    dummy.resize(8, 0);
    for (auto i = 0; i < 2; ++i) {
      for (auto j = 0; j < 2; ++j) {
        psum[i * 4 + j] = data[i * 2 + j];
      }
      as<pnm::uint128_t>(&psum[i * 4 + 2]) = tags[i];
    }
  }

  static auto psum_view(std::vector<uint64_t> &ps) {
    return pnm::make_rowwise_view(ps.data(), ps.data() + ps.size(), 4, 0, 2);
  }
  static auto tag_view(std::vector<uint64_t> &tg) {
    return pnm::make_rowwise_view(tg.data(), tg.data() + tg.size(), 4, 2, 0);
  }
};

TEST_F(SecurePostprocessorSimple, VerificationSingleOk)
NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW {
  const std::vector indices{requests_view{
      0, pnm::make_variable_row_view(index, index + 2,
                                     pnm::make_view(lengths, lengths + 1))}};

  handler->perform_sls(indices, psum_view(psum), tag_view(psum));

  psum[0] = 4;
  psum[1] = 6;
  out.resize(1);

  as<pnm::uint128_t>(&dummy[2]) = tags[0] + tags[1];

  auto check = handler->decrypt_psum(
      pnm::make_rowwise_view(psum.data(), psum.data() + 4, 4, 0, 2),
      pnm::make_rowwise_view(dummy.data(), dummy.data() + 4, 4, 0, 2),
      pnm::make_rowwise_view(psum.data(), psum.data() + 4, 4, 2, 0),
      pnm::make_rowwise_view(dummy.data(), dummy.data() + 4, 4, 2, 0),
      pnm::make_view(out));
  ASSERT_EQ(check, true);
  ASSERT_EQ(out[0], true);
}

TEST_F(SecurePostprocessorSimple, VerificationSingleCorrupted)
NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW {
  const std::vector indices{requests_view{
      0, pnm::make_variable_row_view(index, index + 2,
                                     pnm::make_view(lengths, lengths + 1))}};

  handler->perform_sls(indices, psum_view(psum), tag_view(psum));

  psum[0] = 6;
  psum[1] = 6;
  out.resize(1);

  as<pnm::uint128_t>(&dummy[2]) = tags[0] + tags[1];

  auto check = handler->decrypt_psum(
      pnm::make_rowwise_view(psum.data(), psum.data() + 4, 4, 0, 2),
      pnm::make_rowwise_view(dummy.data(), dummy.data() + 4, 4, 0, 2),
      pnm::make_rowwise_view(psum.data(), psum.data() + 4, 4, 2, 0),
      pnm::make_rowwise_view(dummy.data(), dummy.data() + 4, 4, 2, 0),
      pnm::make_view(out));
  ASSERT_EQ(check, false);
  ASSERT_EQ(out[0], false);
}

TEST_F(SecurePostprocessorSimple, VerificationDoubleBatchOk) {
  const std::vector indices{requests_view{
      0, pnm::variable_row_view(index, index + 2,
                                pnm::make_view(lengths + 1, lengths + 3))}};

  handler->perform_sls(indices, psum_view(psum), tag_view(psum));

  for (auto i = 0; i < 2; ++i) {
    for (auto j = 0; j < 2; ++j) {
      psum[i * 4 + j] = data[i * 2 + j];
    }
  }

  out.resize(2, false);

  as<pnm::uint128_t>(&dummy[2]) = tags[0];
  as<pnm::uint128_t>(&dummy[6]) = tags[1];

  auto check =
      handler->decrypt_psum(psum_view(psum), psum_view(dummy), tag_view(psum),
                            tag_view(dummy), pnm::make_view(out));

  ASSERT_EQ(check, true);
  ASSERT_EQ(out[0], true);
  ASSERT_EQ(out[1], true);
}

TEST_F(SecurePostprocessorSimple, VerificationDoubleBatchCorrupted) {
  const std::vector indices{requests_view{
      0, pnm::variable_row_view(index, index + 2,
                                pnm::make_view(lengths + 1, lengths + 3))}};

  handler->perform_sls(indices, psum_view(psum), tag_view(psum));

  for (auto i = 0; i < 2; ++i) {
    for (auto j = 0; j < 2; ++j) {
      psum[i * 4 + j] = data[i * 2 + j];
    }
  }

  out.resize(2, false);

  as<pnm::uint128_t>(&dummy[2]) = tags[0] - 1;
  as<pnm::uint128_t>(&dummy[6]) = tags[1];

  auto check =
      handler->decrypt_psum(psum_view(psum), psum_view(dummy), tag_view(psum),
                            tag_view(dummy), pnm::make_view(out));

  ASSERT_EQ(check, false);
  ASSERT_EQ(out[0], false);
  ASSERT_EQ(out[1], true);
}

TEST_F(SecurePostprocessorSimple, SimpleEncryption) {
  const std::vector indices{requests_view{
      0, pnm::make_variable_row_view(index, index + 2,
                                     pnm::make_view(lengths, lengths + 1))}};
  handler->perform_sls(indices_t{indices}, psum_view(psum));

  std::vector<uint64_t> epsum{
      edata[0] + edata[2], edata[1] + edata[3], 0, 0, 0, 0, 0, 0};

  handler->decrypt_psum(psum_view(psum), psum_view(epsum));

  ASSERT_EQ(psum[0], 4);
  ASSERT_EQ(psum[1], 6);
}

TEST_F(SecurePostprocessorArtificial, SLSNoMac) {
  sls::secure::DLRMPreprocessor<uint32_t, PtrReader, PtrWriter> preproc(
      data.data(), emb_table.info.rows(), emb_table.info.cols());

  auto postproc = preproc.load(data.data(), false);
  EXPECT_NE(postproc, nullptr);

  std::vector<uint32_t> sls_on_encr;
  sls_on_encr.reserve(golden.size());

  const auto *data_it = data.data();
  for (auto tid = 0UL; tid < emb_table.info.rows().size(); ++tid) {
    auto table_view = pnm::make_rowwise_view(
        data_it, data_it + emb_table.info.rows()[tid] * emb_table.info.cols(),
        emb_table.info.cols());
    auto indices_view = indices_views[tid].second;

    std::advance(data_it, emb_table.info.rows()[tid] * emb_table.info.cols());
    auto v = TestEnv::table_sparse_sls(table_view, indices_view,
                                       emb_table.info.cols());
    sls_on_encr.insert(sls_on_encr.end(), v.begin(), v.end());
  }

  std::vector<uint32_t> encr_psum(golden.size(), 0);
  postproc->perform_sls(
      indices_views, pnm::make_rowwise_view(encr_psum.data(),
                                            encr_psum.data() + encr_psum.size(),
                                            emb_table.info.cols()));

  postproc->decrypt_psum(
      pnm::make_rowwise_view(encr_psum.data(),
                             encr_psum.data() + encr_psum.size(),
                             emb_table.info.cols()),
      pnm::make_rowwise_view(sls_on_encr.data(),
                             sls_on_encr.data() + sls_on_encr.size(),
                             emb_table.info.cols()));

  ASSERT_EQ(encr_psum, golden);
}

TEST_F(SecurePostprocessorArtificial, SLSWithMac) {
  sls::secure::DLRMPreprocessor<uint32_t, PtrReader, PtrWriter> preproc(
      data.data(), emb_table.info.rows(), emb_table.info.cols());

  // output buffer with extra size for tags
  std::vector<uint32_t> enc_data(data.size() * 2, 0);

  auto postproc = preproc.load(enc_data.data(), true);
  EXPECT_NE(postproc, nullptr);

  std::vector<uint32_t> sls_on_encr;
  std::vector<uint32_t> sls_on_tag;
  sls_on_encr.reserve(golden.size());

  static constexpr auto TAG_SIZE = sizeof(pnm::uint128_t) / sizeof(uint32_t);

  const auto *data_it = enc_data.data();
  for (auto tid = 0UL; tid < emb_table.info.rows().size(); ++tid) {
    auto table_view =
        pnm::make_rowwise_view(data_it,
                               data_it + emb_table.info.rows()[tid] *
                                             (emb_table.info.cols() + TAG_SIZE),
                               emb_table.info.cols() + TAG_SIZE, 0, TAG_SIZE);

    auto tag_view = pnm::make_rowwise_view(
        data_it,
        data_it +
            emb_table.info.rows()[tid] * (emb_table.info.cols() + TAG_SIZE),
        emb_table.info.cols() + TAG_SIZE, emb_table.info.cols(), 0);

    std::advance(data_it, emb_table.info.rows()[tid] *
                              (emb_table.info.cols() + TAG_SIZE));

    auto indices_view = indices_views[tid].second;
    auto v = TestEnv::table_sparse_sls(table_view, indices_view,
                                       emb_table.info.cols());
    auto t = TestEnv::table_tag_sls(tag_view, indices_view);

    sls_on_encr.insert(sls_on_encr.end(), v.begin(), v.end());
    sls_on_tag.insert(sls_on_tag.end(), t.begin(), t.end());
  }

  std::vector<uint32_t> encr_psum(golden.size(), 0);
  std::vector<uint32_t> encr_tags(golden.size() / emb_table.info.cols() *
                                  TAG_SIZE);

  postproc->perform_sls(
      indices_views,
      pnm::make_rowwise_view(encr_psum.data(),
                             encr_psum.data() + encr_psum.size(),
                             emb_table.info.cols()),
      pnm::make_rowwise_view(encr_tags.data(),
                             encr_tags.data() + encr_tags.size(), TAG_SIZE));

  std::vector<uint8_t> checks(golden.size() / emb_table.info.cols());
  auto is_ok = postproc->decrypt_psum(
      pnm::make_rowwise_view(encr_psum.data(),
                             encr_psum.data() + encr_psum.size(),
                             emb_table.info.cols()),
      pnm::make_rowwise_view(sls_on_encr.data(),
                             sls_on_encr.data() + sls_on_encr.size(),
                             emb_table.info.cols()),
      pnm::make_rowwise_view(encr_tags.data(),
                             encr_tags.data() + encr_tags.size(), TAG_SIZE),
      pnm::make_rowwise_view(sls_on_tag.data(),
                             sls_on_tag.data() + sls_on_tag.size(), TAG_SIZE),
      pnm::make_view(checks));

  EXPECT_EQ(encr_psum, golden);

  EXPECT_TRUE(is_ok);
  EXPECT_TRUE(
      std::all_of(checks.begin(), checks.end(), [](auto v) { return v; }));
}

TEST_F(SecurePostprocessorArtificial, SLSWithMacCorrupted) {
  sls::secure::DLRMPreprocessor<uint32_t, PtrReader, PtrWriter> preproc(
      data.data(), emb_table.info.rows(), emb_table.info.cols());

  // output buffer with extra size for tags
  std::vector<uint32_t> enc_data(data.size() * 2, 0);

  auto postproc = preproc.load(enc_data.data(), true);
  EXPECT_NE(postproc, nullptr);

  std::vector<uint32_t> sls_on_encr;
  std::vector<uint32_t> sls_on_tag;
  sls_on_encr.reserve(golden.size());

  static constexpr auto TAG_SIZE = sizeof(pnm::uint128_t) / sizeof(uint32_t);

  constexpr std::array corrupted_table{0, 3};

  auto *data_it = enc_data.data();
  for (auto tid = 0UL; tid < emb_table.info.rows().size(); ++tid) {
    auto table_view = pnm::make_rowwise_view<const uint32_t>(
        data_it,
        data_it +
            emb_table.info.rows()[tid] * (emb_table.info.cols() + TAG_SIZE),
        emb_table.info.cols() + TAG_SIZE, 0, TAG_SIZE);

    auto tag_view = pnm::make_rowwise_view<const uint32_t>(
        data_it,
        data_it +
            emb_table.info.rows()[tid] * (emb_table.info.cols() + TAG_SIZE),
        emb_table.info.cols() + TAG_SIZE, emb_table.info.cols(), 0);

    if (std::count(corrupted_table.begin(), corrupted_table.end(), tid)) {
      std::fill(data_it,
                data_it + emb_table.info.rows()[tid] *
                              (emb_table.info.cols() + TAG_SIZE),
                42);
    }

    std::advance(data_it, emb_table.info.rows()[tid] *
                              (emb_table.info.cols() + TAG_SIZE));

    auto indices_view = indices_views[tid].second;
    auto v = TestEnv::table_sparse_sls(table_view, indices_view,
                                       emb_table.info.cols());
    auto t = TestEnv::table_tag_sls(tag_view, indices_view);

    sls_on_encr.insert(sls_on_encr.end(), v.begin(), v.end());
    sls_on_tag.insert(sls_on_tag.end(), t.begin(), t.end());
  }

  std::vector<uint32_t> encr_psum(golden.size(), 0);
  std::vector<uint32_t> encr_tags(golden.size() / emb_table.info.cols() *
                                  TAG_SIZE);

  postproc->perform_sls(
      indices_views,
      pnm::make_rowwise_view(encr_psum.data(),
                             encr_psum.data() + encr_psum.size(),
                             emb_table.info.cols()),
      pnm::make_rowwise_view(encr_tags.data(),
                             encr_tags.data() + encr_tags.size(), TAG_SIZE));

  std::vector<uint8_t> checks(golden.size() / emb_table.info.cols());
  auto is_ok = postproc->decrypt_psum(
      pnm::make_rowwise_view(encr_psum.data(),
                             encr_psum.data() + encr_psum.size(),
                             emb_table.info.cols()),
      pnm::make_rowwise_view(sls_on_encr.data(),
                             sls_on_encr.data() + sls_on_encr.size(),
                             emb_table.info.cols()),
      pnm::make_rowwise_view(encr_tags.data(),
                             encr_tags.data() + encr_tags.size(), TAG_SIZE),
      pnm::make_rowwise_view(sls_on_tag.data(),
                             sls_on_tag.data() + sls_on_tag.size(), TAG_SIZE),
      pnm::make_view(checks));

  EXPECT_FALSE(is_ok);
  auto psum_view = pnm::make_rowwise_view(encr_psum.data(),
                                          encr_psum.data() + encr_psum.size(),
                                          emb_table.info.cols());
  auto golden_view = pnm::make_rowwise_view(
      golden.data(), golden.data() + golden.size(), emb_table.info.cols());
  auto i = 0U;
  for (auto [tid, batches] : indices_views) {
    for ([[maybe_unused]] auto _ : batches) {
      if (std::count(corrupted_table.begin(), corrupted_table.end(), tid)) {
        EXPECT_FALSE(checks[i]);
      } else {
        ASSERT_TRUE(std::equal((psum_view.begin() + i)->begin(),
                               (psum_view.begin() + i)->end(),
                               (golden_view.begin() + i)->begin()));
      }
      ++i;
    }
  }
}
