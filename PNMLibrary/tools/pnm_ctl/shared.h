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

#ifndef TOOLS_CTL_SHARED_H
#define TOOLS_CTL_SHARED_H

#include "command_interface.h"

#include "CLI/App.hpp"
#include "CLI/CLI.hpp" // NOLINT(misc-include-cleaner)

#include <cstdint>

namespace tools::ctl {

// [TODO: @e-kutovoi] Put this in pnm_sls_mem_topology.h
constexpr auto SLS_SHMEM = "/sls";

class DestroySharedMemory : public ICommand {
public:
  void add_subcommand(CLI::App &app) override;

private:
  void destroy_shm(const char *name);
  void execute();

  bool is_sls_{};
  bool is_imdb_{};
};

class SetupSharedMemory : public ICommand {
public:
  void add_subcommand(CLI::App &app) override;

private:
  void make_shm(const char *name, uint64_t size);
  void execute();

  bool is_sls_{};
  bool is_imdb_{};
  uint64_t mem_size_{};
  uint64_t regs_size_{};
};

} // namespace tools::ctl

#endif // TOOLS_CTL_SHARED_H
