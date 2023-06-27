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

#ifndef TOOLS_CTL_SLS_H
#define TOOLS_CTL_SLS_H

#include "command_interface.h"

#include "pnmlib/core/sls_cunit_info.h"

#include "CLI/App.hpp"
#include "CLI/CLI.hpp" // NOLINT(misc-include-cleaner)
#include "CLI/Option.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace tools::ctl::sls {

namespace details {

struct RankInfoDescription {
  std::string option_name;
  std::string option_description;
  std::string printable_name;
  SlsComputeUnitInfo compute_unit_info;
};

} // namespace details

class PrintInfo final : public ICommand {
public:
  void add_subcommand(CLI::App &app) override;

private:
  void execute();

  static const std::array<details::RankInfoDescription, cunit_info_length>
      descriptions_;

  std::vector<uint64_t> enabled_ranks_;

  CLI::Option *is_hex_{};
  std::vector<CLI::Option *> rank_info_options_{};
  CLI::App *sub_ = nullptr;
  bool sub_is_initialized_ = false;
};

class Reset final : public ICommand {
public:
  void add_subcommand(CLI::App &app) override;
};

class AcquisitionTimeout final : public ICommand {
public:
  void add_subcommand(CLI::App &app) override;

private:
  uint64_t set_timeout_{};
};

class ResourceCleanup final : public ICommand {
public:
  void add_subcommand(CLI::App &app) override;

private:
  static constexpr auto ON = "on";
  static constexpr auto OFF = "off";

  uint64_t set_cleanup_{};
};

class LeakedInfo final : public ICommand {
public:
  void add_subcommand(CLI::App &app) override;
};

bool check_sls_available();
} // namespace tools::ctl::sls

#endif // TOOLS_CTL_SLS_H
