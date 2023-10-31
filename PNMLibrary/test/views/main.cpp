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

#include "pnmlib/common/rowwise_view.h"
#include "pnmlib/common/variable_row_view.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <fmt/core.h>
#include <fmt/format.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <iterator>
#include <numeric>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

using pnm::views::common;
using pnm::views::make_rowwise_view;
using pnm::views::make_variable_row_view;
using pnm::views::make_view;

constexpr char cmem[] = "The brown fox jumps over lazy dog";
constexpr auto N = sizeof(cmem);

TEST(View, SequentialConstMemory) {
  common full_range{cmem, cmem + N};
  static_assert(std::is_same_v<decltype(full_range.begin()), const char *>,
                "CTAD Failed");
  ASSERT_EQ(full_range.size(), N);
  ASSERT_TRUE(std::equal(full_range.begin(), full_range.end(), cmem, cmem + N));

  common slice{cmem + 4, cmem + 9};
  ASSERT_EQ(slice.size(), 5);
  ASSERT_TRUE(std::equal(slice.begin(), slice.end(), "brown"));

  ASSERT_EQ(fmt::format("{}", fmt::join(slice, "")), std::string{"brown"});

  auto fox = make_view(cmem + 10, 3);
  static_assert(std::is_same_v<decltype(fox.begin()), const char *>,
                "FTAD failed");
  ASSERT_TRUE(std::equal(fox.begin(), fox.end(), "fox"));
}

TEST(View, SequentialMutableMemory) {
  char mem[N];
  std::copy_n(cmem, N, mem);

  common full_range{mem, mem + N};
  static_assert(std::is_same_v<decltype(full_range.begin()), char *>,
                "CTAD Failed");

  ASSERT_EQ(full_range.size(), N);
  ASSERT_TRUE(std::equal(full_range.begin(), full_range.end(), mem, mem + N));

  common slice{mem + 4, mem + 9};
  ASSERT_EQ(slice.size(), 5);
  ASSERT_TRUE(std::equal(slice.begin(), slice.end(), "brown"));

  ASSERT_EQ(fmt::format("{}", fmt::join(slice, "")), std::string{"brown"});

  std::transform(slice.begin(), slice.end(), slice.begin(),
                 [](auto c) { return std::toupper(c); });
  ASSERT_TRUE(std::equal(slice.begin(), slice.end(), "BROWN"));
  ASSERT_TRUE(std::equal(mem, mem + N, "The BROWN fox jumps over lazy dog"));
}

TEST(View, ContainerConst) {
  std::vector mem(cmem, cmem + N);
  const auto &cref = mem;

  auto full = make_view(cref);
  static_assert(std::is_same_v<decltype(full.begin()), const char *>,
                "FTAD Failed");

  ASSERT_TRUE(std::equal(mem.begin(), mem.end(), full.begin()));
  ASSERT_TRUE(std::equal(mem.begin(), mem.end(), make_view(mem).begin()));

  [[maybe_unused]] auto slice = make_view(&cref[4], &cref[9]);
  static_assert(std::is_same_v<decltype(slice.begin()), const char *>,
                "FTAD Failed");
}

TEST(View, ContainerMutable) {
  std::vector mem(cmem, cmem + N);

  auto full = make_view(mem);
  static_assert(std::is_same_v<decltype(full.begin()), char *>, "FTAD Failed");

  ASSERT_TRUE(std::equal(mem.begin(), mem.end(), full.begin()));
  ASSERT_TRUE(std::equal(mem.begin(), mem.end(), make_view(mem).begin()));

  auto slice = make_view(&mem[4], &mem[9]);
  static_assert(std::is_same_v<decltype(slice.begin()), char *>, "FTAD Failed");

  std::copy_n("BROWN", 5, slice.begin());
  ASSERT_TRUE(
      std::equal(mem.begin(), mem.end(), "The BROWN fox jumps over lazy dog"));
}

TEST(View, FunctionCalls) {
  std::vector mem{1, 2, 3, 4, 5, 6, 7};
  const auto &cref = mem;

  auto rng_value = [](common<int> r) {
    std::transform(r.begin(), r.end(), r.begin(), r.begin(), std::plus{});
  };
  static_assert(
      !std::is_invocable_v<decltype(rng_value), decltype(make_view(cref))>,
      "FTAD Failed. Check make_view func.");

  rng_value(make_view(mem));
  ASSERT_EQ(mem, (std::vector{2, 4, 6, 8, 10, 12, 14}));

  auto rng_const_value = [](common<const int> r) {
    return std::accumulate(r.begin(), r.end(), 0);
  };
  static_assert(
      std::is_invocable_v<decltype(rng_const_value), decltype(make_view(cref))>,
      "FTAD Failed. Check make_view func.");
  ASSERT_EQ(rng_const_value(make_view(cref)), 56);

  [[maybe_unused]] auto rng_ref = [](common<int> &r) {
    std::transform(r.begin(), r.end(), r.begin(), r.begin(), std::plus{});
  };
  [[maybe_unused]] auto rng_cref = [](const common<int> &r) {
    fmt::print("{}", fmt::join(r, " "));
  };

  [[maybe_unused]] auto rng = make_view(mem);
  static_assert(std::is_invocable_v<decltype(rng_ref), decltype(rng) &>,
                "Fail to pass over ref");
  static_assert(std::is_invocable_v<decltype(rng_cref), const decltype(rng) &>,
                "Fail to pass over cref");
}

TEST(ViewArray, CreateReadonly) {
  auto *ptr = new int[10]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  const auto *cptr = new const int[10]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  auto full = make_rowwise_view(ptr, ptr + 10, 5);
  auto cfull = make_rowwise_view(cptr, cptr + 10, 5);

  auto *it = ptr;
  for (auto view : full) {
    static_assert(std::is_same_v<decltype(view.begin()), int *>,
                  "Type deduction failed in function make_view");
    fmt::print("{}\n", fmt::join(view, " "));
    ASSERT_TRUE(std::equal(it, it + 5, view.begin()));
    it += 5;
  }

  const auto *cit = cptr;
  for (auto view : cfull) {
    static_assert(std::is_same_v<decltype(view.begin()), const int *>,
                  "Type deduction failed in function make_view");
    fmt::print("{}\n", fmt::join(view, " "));
    ASSERT_TRUE(std::equal(cit, cit + 5, view.begin()));
    cit += 5;
  }

  delete[] ptr;
  delete[] cptr;
}

TEST(ViewArray, CreateSliceReadonly) {
  auto *ptr = new int[10]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  auto full = make_rowwise_view(ptr, ptr + 10, 5, 3, 0);

  auto *it = ptr;
  for (auto view : full) {
    static_assert(std::is_same_v<decltype(view.begin()), int *>,
                  "Type deduction failed in function make_view");
    fmt::print("{}\n", fmt::join(view, " "));
    ASSERT_TRUE(std::equal(it + 3, it + 5, view.begin()));
    it += 5;
  }

  delete[] ptr;
}

TEST(ViewArray, CreateSliceModify) {
  std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  auto vv = make_rowwise_view(data.data(), data.data() + 10, 5, 3, 0);

  for (auto view : vv) {
    std::transform(view.begin(), view.end(), view.begin(), view.begin(),
                   std::multiplies{});
  }

  std::vector<int> data_sqr = {1, 2, 3, 16, 25, 6, 7, 8, 81, 100};

  ASSERT_TRUE(std::equal(data.data(), data.data() + 10, data_sqr.data()));
}

TEST(View, ConstBeginEnd) {
  int array[] = {1, 2, 3, 4, 5, 6, 7};
  auto nc_array_v = make_view(array, 6);
  const auto &array_v = nc_array_v;

  // Don't use `fmt::join` to test the provided interface precisely.
  for (const int item : array_v) {
    fmt::print("{} ", item);
  }
  static_assert(std::is_same_v<decltype(array_v.begin()), const int *>);
  static_assert(std::is_same_v<decltype(array_v.end()), const int *>);
}

TEST(ViewArray, ConstBeginEnd) {
  int array[] = {1, 2, 3, 4, 5, 6, 7, 8};
  auto nc_av = make_rowwise_view(array, array + 8, 4);
  const auto &av = nc_av;
  auto it = av.begin();

  static_assert(std::is_same_v<decltype(it.operator*()), common<const int> &>);

  for (; it != av.end(); ++it) {
    const auto &sv = *it;
    for (const auto *sit = sv.begin(); sit != sv.end(); ++sit) {
      static_assert(std::is_same_v<decltype(sit), const int *>);
      fmt::print("{} ", *sit);
    }
    fmt::print("\n");
  }
}

TEST(ViewArray, PsumWithTag) {
  constexpr static auto PSUM_NUM = 10;
  constexpr static auto COLS_NUM = 16;

  using tag_type = uint64_t;
  constexpr static auto TAG_COLS = sizeof(tag_type) / sizeof(uint32_t);

  std::vector<uint32_t> data(PSUM_NUM *
                             (COLS_NUM + TAG_COLS)); // memory for PSUM in sls

  // Generate tags
  {
    auto psum_view = make_rowwise_view(data.data(), data.data() + data.size(),
                                       COLS_NUM + TAG_COLS, 0, TAG_COLS);
    auto tag_view = make_rowwise_view(data.data(), data.data() + data.size(),
                                      TAG_COLS + COLS_NUM, COLS_NUM, 0);
    uint32_t start_value = 0;
    auto gen_psum_with_tag = [&start_value](auto &psum_v, auto &tag_v) {
      std::iota(psum_v.begin(), psum_v.end(), start_value);
      start_value += psum_v.size();

      auto &tag = *reinterpret_cast<uint64_t *>(tag_v.begin());
      tag = std::accumulate(psum_v.begin(), psum_v.end(), 0ULL);
      tag = (tag << 32) - tag;
      return 0;
    };
    std::inner_product(psum_view.begin(), psum_view.end(), tag_view.begin(), 0,
                       std::plus{}, gen_psum_with_tag);
  }

  // print data
  {
    auto data_v = make_rowwise_view(data.data(), data.data() + data.size(),
                                    COLS_NUM + TAG_COLS);

    auto pretty_print = [](auto x) {
      fmt::print("{}\t", static_cast<int32_t>(x));
    };

    for (const auto &row_v : data_v) {
      std::for_each(row_v.begin(), row_v.end(), pretty_print);
      fmt::print("\n");
    }
  }

  // Read and check data
  {
    auto psum_view = make_rowwise_view(data.data(), data.data() + data.size(),
                                       COLS_NUM + TAG_COLS, 0, TAG_COLS);
    auto tag_view = make_rowwise_view(data.data(), data.data() + data.size(),
                                      TAG_COLS + COLS_NUM, COLS_NUM, 0);
    auto psum_it = psum_view.begin();
    auto tag_it = tag_view.begin();
    for (; psum_it != psum_view.end(); ++psum_it, ++tag_it) {
      auto gtag = std::accumulate(psum_it->begin(), psum_it->end(), 0ULL);
      gtag = (gtag << 32) - gtag;
      auto tag = *reinterpret_cast<const uint64_t *>(tag_it->begin());
      ASSERT_EQ(gtag, tag);
    }
  }

  // Same stuff but with std::vector
  {
    std::vector<common<uint32_t>> psum_view(PSUM_NUM);
    std::vector<common<uint32_t>> tag_view(PSUM_NUM);

    for (auto col = 0; col < PSUM_NUM; ++col) {
      auto shift = (COLS_NUM + TAG_COLS) * col;
      auto row_size = COLS_NUM + TAG_COLS;
      psum_view[col] = make_view(data.data() + shift,
                                 data.data() + shift + row_size - TAG_COLS);
      tag_view[col] = make_view(data.data() + shift + COLS_NUM,
                                data.data() + shift + row_size);
    }

    auto psum_it = psum_view.begin();
    auto tag_it = tag_view.begin();
    for (; psum_it != psum_view.end(); ++psum_it, ++tag_it) {
      auto gtag = std::accumulate(psum_it->begin(), psum_it->end(), 0ULL);
      gtag = (gtag << 32) - gtag;
      auto tag = *reinterpret_cast<const uint64_t *>(tag_it->begin());
      ASSERT_EQ(gtag, tag);
    }
  }
}

TEST(VariableRowView, CreateAndNavigate) {
  uint64_t length[] = {4, 6, 4, 6, 5, 5, 4};
  static constexpr auto LS = sizeof(length) / sizeof(length[0]);
  auto view =
      make_variable_row_view(cmem, cmem + N, make_view(length, length + LS));
  auto i = 0;
  const auto *ptr = cmem;
  for (auto str_view : view) {
    ASSERT_EQ(str_view.size(), length[i]);
    auto golden = make_view(ptr, ptr + length[i]);
    ASSERT_TRUE(std::equal(str_view.begin(), str_view.end(), golden.begin()));
    fmt::print("{}\n", fmt::join(str_view, ""));
    std::advance(ptr, length[i++]);
  }

  auto vit = view.begin();
  static constexpr auto ADVANCE_STEP = 4;

  // move to 'over ' substr, just a random value
  std::advance(vit, ADVANCE_STEP);
  ASSERT_EQ(vit->size(), length[ADVANCE_STEP]);

  fmt::print("{}\n", fmt::join(*vit, ""));
  ASSERT_EQ(std::string_view(vit->begin(), length[ADVANCE_STEP]),
            std::string_view("over "));

  --vit;
  ASSERT_EQ(vit->size(), length[ADVANCE_STEP - 1]);

  fmt::print("{}\n", fmt::join(*vit, ""));
  ASSERT_EQ(std::string_view(vit->begin(), length[ADVANCE_STEP - 1]),
            std::string_view("jumps "));

  std::advance(vit, -ADVANCE_STEP + 1);
  ASSERT_EQ(vit->size(), length[0]);
  fmt::print("{}\n", fmt::join(*vit, ""));
  ASSERT_EQ(std::string_view(vit->begin(), length[0]),
            std::string_view("The "));

  auto cit = std::cref(view).get().begin();
  fmt::print("{}\n", fmt::join(*cit, ""));
}
