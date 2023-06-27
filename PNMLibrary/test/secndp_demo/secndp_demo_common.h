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

#ifndef _SECNDP_DEMO_COMMON_H
#define _SECNDP_DEMO_COMMON_H

#include "common/mapped_file.h"
#include "common/profile.h"

#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/utils.h"

#include "pnmlib/common/misc_utils.h"
#include "pnmlib/common/rowwise_view.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <mutex>
#include <numeric>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T> inline constexpr auto UnitStr = "Undefined";
template <> inline constexpr auto UnitStr<std::chrono::milliseconds> = "ms";

inline std::mutex summary_mutex;

/*! Class to make working time measure and logging.
 *[TODO: @y-lavrinenko] Replace by common API if it will be available
 * */
template <typename Labels = std::string,
          typename Duration = std::chrono::milliseconds>
class TimeStatistics {
public:
  explicit TimeStatistics(Labels global_section = {})
      : global_section_{std::move(global_section)} {
    section_timers_[global_section_].first.tick();
  }

  void start_section(const Labels &l) {
    section_timers_[l].first.tick();
    section_timers_[l].second = false;
  }

  void end_section(const Labels &l) {
    section_timers_.at(l).first.tock();
    section_timers_.at(l).second = true;
  }

  void summarize() const {
    {
      const std::lock_guard guard(summary_mutex);
      fmt::print("Working time log:\n");
      fmt::print("{:<30}{:<15}{:<10}\n", "Label", "Elapsed time", "Units");
    }
    for (const auto &[lable, timer] : section_timers_) {
      if (timer.second) {
        dump_wtime(lable, timer.first.duration());
      }
    }
  }

  void reset() { section_timers_.clear(); }

  ~TimeStatistics() {
    if (global_section_ != Labels{}) {
      section_timers_[global_section_].first.tock();
      dump_wtime(global_section_,
                 section_timers_[global_section_].first.duration());
    }
  }

private:
  void dump_wtime(const Labels &label, Duration dur) const {
    const std::lock_guard guard(summary_mutex);
    fmt::print("{:<30}{:<15}{:<10}\n", label, dur.count(), UnitStr<Duration>);
  }

  using timer_t =
      pnm::profile::Timer<Duration, std::chrono::high_resolution_clock>;

  std::unordered_map<Labels, std::pair<timer_t, bool>> section_timers_;
  Labels global_section_;
};

/*! Class for loading embedded tables and indices.
 * @tparam T -- table's entry type*/
template <typename T> class TestLoader {
public:
  TestLoader(const std::filesystem::path &root,
             const std::string &indices_set_name) {
    load_all(root, indices_set_name);
  }

  const auto &tables() const { return tables_; }
  const auto &tables_meta() const { return tables_meta_; }

  const auto &indices() const { return indices_; }
  const auto &indices_meta() const { return indices_meta_; }

  const auto &golden() const { return golden_; }

protected:
  void load_all(const std::filesystem::path &root,
                const std::string &indices_set_name) {
    TimeStatistics t("LOAD TEST DATA");
    fmt::print("Loading embedded tables...\n");
    t.start_section("- Tables load");
    ASSERT_NO_THROW(load_tables(root))
        << "Fail to load embedded tables! Root: " << root
        << "\tIndices set name: " << indices_set_name;
    t.end_section("- Tables load");
    static constexpr auto tinfo_str = R"(Done. Load {} bytes.
Tables structure:
  sparse_feature_size: {}
  entry type size: {}
)";
    fmt::print(tinfo_str, pnm::utils::byte_size_of_container(tables_),
               tables_meta_.cols(), sizeof(T));

    fmt::print("Loading indices...\n");
    t.start_section("- Indices load");
    ASSERT_NO_THROW(load_indices(root, indices_set_name))
        << "Fail to load indices! Root: " << root
        << "\tIndices set name: " << indices_set_name;
    t.end_section("- Indices load");
    fmt::print("Done. Load {} indices.\n", indices_.size());

    fmt::print("Loading golden vector...\n");
    t.start_section("- Golden vector load");
    ASSERT_NO_THROW(load_golden(root, indices_set_name))
        << "Fail to load golden vector! Root: " << root
        << "\tIndices set name: " << indices_set_name;
    t.end_section("- Golden vector load");
    fmt::print("Done. Load {} bytes ({} rows)\n",
               pnm::utils::byte_size_of_container(golden_),
               golden_.size() / tables_meta_.cols());

    t.summarize();
  }

  void load_tables(const std::filesystem::path &root) {
    auto thandler = sls::tests::get_test_tables_mmap(root);
    auto table_view = thandler.mapped_file.get_view<uint8_t>();
    tables_ = std::vector<uint8_t>(table_view.begin(), table_view.end());
    tables_meta_ = std::move(thandler.info);
  }

  void load_indices(const std::filesystem::path &root,
                    const std::string &indices_set_name) {
    auto ihandler = sls::tests::get_test_indices(root, indices_set_name);
    auto index_count = std::accumulate(ihandler.info.lengths().begin(),
                                       ihandler.info.lengths().end(), 0ULL);
    indices_.resize(index_count);
    ihandler.in.read(reinterpret_cast<char *>(indices_.data()),
                     pnm::utils::byte_size_of_container(indices_));
    indices_meta_ = std::move(ihandler.info);
  }

  void load_golden(const std::filesystem::path &root,
                   const std::string &indices_set_name) {
    auto in = sls::tests::get_golden_vector(root, indices_set_name);
    auto size = stream_size(in);
    golden_.resize(size / sizeof(T));
    in.read(reinterpret_cast<char *>(golden_.data()), size);
  }

  auto stream_size(std::ifstream &in) const {
    in.seekg(0, std::ios_base::end);
    auto size = in.tellg();
    in.seekg(0, std::ios_base::beg);
    return size;
  }

private:
  std::vector<uint8_t> tables_;
  TablesInfo tables_meta_;

  std::vector<uint32_t> indices_;
  IndicesInfo indices_meta_;

  std::vector<T> golden_;
};

/*! Struct to organize useful functions that used in demo app
 * */
struct Utils {
  template <typename T>
  static auto compare_with_golden(const std::vector<T> &golden,
                                  const std::vector<T> &psum, size_t cols,
                                  const std::vector<uint8_t> &check,
                                  bool has_tag) {
    auto psum_v =
        pnm::rowwise_view(psum.data(), psum.data() + psum.size(), cols);
    auto golden_v =
        pnm::rowwise_view(golden.data(), golden.data() + golden.size(), cols);

    auto ok_entries = 0;
    auto check_row = check.begin();
    for (auto it1 = psum_v.begin(), it2 = golden_v.begin(); it1 != psum_v.end();
         ++it1, ++it2, ++check_row) {
      if (has_tag) {
        EXPECT_TRUE(*check_row);
      }
      EXPECT_TRUE(std::equal(it1->begin(), it1->end(), it2->begin()));
      ok_entries += std::inner_product(it1->begin(), it1->end(), it2->begin(),
                                       0, std::plus{}, std::equal_to{});
    }
    EXPECT_EQ(ok_entries, golden.size());
    fmt::print("Test passed! (IsOk/Total) ({}/{})\n", ok_entries,
               golden.size());
  }
};

#endif // _SECNDP_DEMO_COMMON_H
