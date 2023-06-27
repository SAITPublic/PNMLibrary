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

#ifndef _SLS_TEST_UTILS_H
#define _SLS_TEST_UTILS_H

#include <fmt/core.h>

#include <linux/sls_resources.h>

#include <cerrno>
#include <cstdio>
#include <fstream>
#include <ios>
#include <iosfwd>
#include <limits>
#include <stdexcept>
#include <string>

#define CHECK_RETVAL_EQ(func, expected_retval)                                 \
  do {                                                                         \
    if (func != expected_retval)                                               \
      fmt::print(stderr, "{}\n", strerror(errno));                             \
  } while (false)

namespace test_app {

inline std::string preference_to_str(sls_user_preferences pref) {
  switch (pref) {
  case SLS_ALLOC_AUTO:
    return "AUTO";
  case SLS_ALLOC_REPLICATE_ALL:
    return "REPLICATE_ALL";
  case SLS_ALLOC_DISTRIBUTE_ALL:
    return "DISTRIBUTE_ALL";
  case SLS_ALLOC_SINGLE:
    return "SINGLE";
  };
  throw std::runtime_error(fmt::format("Invalid memory user preference: {}\n",
                                       static_cast<int>(pref)));
}

inline unsigned long get_mem_avail_in_bytes() {
  static constexpr auto MEMINFO_FILE = "/proc/meminfo";
  static constexpr auto MEM_AVAIL_TOKEN = "MemAvailable:";
  auto avail_mem_in_bytes = 0LU;
  std::string token;
  std::ifstream file(MEMINFO_FILE);
  while (file >> token) {
    if (token == MEM_AVAIL_TOKEN) {
      return file >> avail_mem_in_bytes ? avail_mem_in_bytes * 1024 : 0LU;
    }
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return 0LU;
}

inline std::streampos stream_size(std::ifstream &in) {
  in.seekg(0, std::ios_base::end);
  auto size = in.tellg();
  in.seekg(0, std::ios_base::beg);
  return size;
}
} // namespace test_app

#endif // _SLS_TEST_UTILS_H
