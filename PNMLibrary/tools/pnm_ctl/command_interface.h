/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
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

#ifndef TOOLS_CTL_COMMAND_INTERFACE_H
#define TOOLS_CTL_COMMAND_INTERFACE_H

#include "pnmlib/common/error.h"

#include "CLI/App.hpp"
#include "CLI/CLI.hpp" // NOLINT(misc-include-cleaner)

#include <fmt/core.h>

#include <unistd.h>

#include <cstdio>

namespace tools::ctl {

inline void check_privileges() {
  if (geteuid() != 0) {
    throw pnm::error::IO("Run \"pnm_ctl\" with superuser privileges.");
  }
}

template <typename... Args>
inline void print_err(fmt::format_string<Args...> fmt, Args &&...args) {
  fmt::print(stderr, fmt, std::forward<Args>(args)...);
}

struct ICommand {
  virtual void add_subcommand(CLI::App &app) = 0;
  virtual ~ICommand() = default;
};

} // namespace tools::ctl

#endif // TOOLS_CTL_COMMAND_INTERFACE_H
