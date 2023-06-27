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

#ifndef PNM_MAKE_ERROR_H
#define PNM_MAKE_ERROR_H

#include "common/error_message.h"

#include "pnmlib/common/error.h"

#include <fmt/core.h>

#include <cerrno>
#include <cstring>
#include <string_view>
#include <utility>

namespace pnm::error {

template <typename Exception, typename... Args>
auto make(fmt::format_string<Args...> fmt, Args &&...args) {
  return Exception(fmt::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
auto make_oom(fmt::format_string<Args...> fmt, Args &&...args) {
  return make<OutOfMemory>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
auto make_bad(fmt::format_string<Args...> fmt, Args &&...args) {
  return make<BadAccess>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
auto make_inval(fmt::format_string<Args...> fmt, Args &&...args) {
  return make<InvalidArguments>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
auto make_io(fmt::format_string<Args...> fmt, Args &&...args) {
  return make<IO>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
auto make_not_sup(fmt::format_string<Args...> fmt, Args &&...args) {
  return make<NotSupported>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
auto make_fail(fmt::format_string<Args...> fmt, Args &&...args) {
  return make<Failure>(fmt, std::forward<Args>(args)...);
}

inline void throw_from_errno(int err,
                             const std::string_view extra_info = from_errno) {
  auto message = fmt::format(extra_info, strerror(err));

  switch (err) {
  case EINVAL:
    throw pnm::error::InvalidArguments(message);
  case ENOMEM:
    throw pnm::error::OutOfMemory(message);
  case EIO:
    throw pnm::error::IO(message);
  default:
    throw pnm::error::Failure(message);
  }
}

} // namespace pnm::error

#endif // PNM_MAKE_ERROR_H
