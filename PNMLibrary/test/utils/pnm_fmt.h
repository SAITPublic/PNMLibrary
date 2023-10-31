/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#ifndef _PNM_FMT_H_
#define _PNM_FMT_H_

#include "pnmlib/core/accessor.h"
#include "pnmlib/core/device.h"

#include <fmt/core.h>
#include <fmt/format.h>

#include <linux/sls_resources.h>

#include <stdexcept>
#include <string>

// IWYU pragma: always_keep

template <typename T>
struct fmt::formatter<pnm::memory::AccessorProxyRef<T>> : fmt::formatter<T> {
  auto format(T ref, format_context &ctx) const {
    return formatter<T>::format(ref, ctx);
  }
};

template <>
struct fmt::formatter<pnm::Device::Type> : fmt::formatter<std::string> {
  auto format(pnm::Device::Type type, format_context &ctx) const {
    switch (type) {
    case pnm::Device::Type::SLS:
      return fmt::formatter<std::string>::format("SLS", ctx);
    case pnm::Device::Type::IMDB:
      return fmt::formatter<std::string>::format("IMDB", ctx);
    }

    throw std::runtime_error(
        fmt::format("Unknown device type {}.", fmt::underlying(type)));
  }
};

inline std::string format_as(sls_user_preferences pref) {
  switch (pref) {
  case SLS_ALLOC_AUTO:
    return "AUTO";
  case SLS_ALLOC_REPLICATE_ALL:
    return "REPLICATE_ALL";
  case SLS_ALLOC_DISTRIBUTE_ALL:
    return "DISTRIBUTE_ALL";
  case SLS_ALLOC_SINGLE:
    return "SINGLE";
  }
  throw std::runtime_error(fmt::format("Invalid memory user preference: {}\n",
                                       fmt::underlying(pref)));
}

constexpr auto print_fmt_test_params = [](const auto &info) {
  return fmt::format("{}", info.param);
};

#endif // _PNM_FMT_H_
