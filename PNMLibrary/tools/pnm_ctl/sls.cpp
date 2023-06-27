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

#include "sls.h"

#include "common/topology_constants.h"

#include "tools/pnm_ctl/command_interface.h"

#include "pnmlib/core/device.h"
#include "pnmlib/core/sls_cunit_info.h"
#include "pnmlib/core/sls_device.h"

#include "pnmlib/common/error.h"

#include "CLI/App.hpp"
#include "CLI/Option.hpp"
#include "CLI/Validators.hpp"

#include <fmt/core.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <numeric>
#include <string>

using namespace tools::ctl::sls;

const std::array<details::RankInfoDescription, cunit_info_length>
    PrintInfo::descriptions_ = {
        details::RankInfoDescription{"--state",
                                     "Show state of rank (0 = free, 1 = busy)",
                                     "STATE:", SlsComputeUnitInfo::State},
        details::RankInfoDescription{
            "--acquisition-count", "Show acquisition count of rank",
            "ACQUISITION COUNT:", SlsComputeUnitInfo::AcquisitionCount},
        details::RankInfoDescription{
            "--free-size", "Show size of free rank memory in bytes",
            "FREE SIZE:", SlsComputeUnitInfo::FreeSize},
        details::RankInfoDescription{
            "--base-size", "Show size of BASE region in bytes",
            "BASE SIZE:", SlsComputeUnitInfo::RegionBaseSize},
        details::RankInfoDescription{
            "--base-offset", "Show offset of BASE region in bytes",
            "BASE OFFSET:", SlsComputeUnitInfo::RegionBaseOffset},
        details::RankInfoDescription{
            "--inst-size", "Show size of INST region in bytes",
            "INST SIZE:", SlsComputeUnitInfo::RegionInstSize},
        details::RankInfoDescription{
            "--inst-offset", "Show offset of INST region in bytes",
            "INST OFFSET:", SlsComputeUnitInfo::RegionInstOffset},
        details::RankInfoDescription{
            "--cfgr-size", "Show size of CFGR region in bytes",
            "CFGR SIZE:", SlsComputeUnitInfo::RegionCfgrSize},
        details::RankInfoDescription{
            "--cfgr-offset", "Show offset of CFGR region in bytes",
            "CFGR OFFSET:", SlsComputeUnitInfo::RegionCfgrOffset},
        details::RankInfoDescription{
            "--tags-size", "Show size of TAGS region in bytes",
            "TAGS SIZE:", SlsComputeUnitInfo::RegionTagsSize},
        details::RankInfoDescription{
            "--tags-offset", "Show offset of TAGS region in bytes",
            "TAGS OFFSET:", SlsComputeUnitInfo::RegionTagsOffset},
        details::RankInfoDescription{
            "--psum-size", "Show size of PSUM region in bytes",
            "PSUM SIZE:", SlsComputeUnitInfo::RegionPsumSize},
        details::RankInfoDescription{
            "--psum-offset", "Show offset of PSUM region in bytes",
            "PSUM OFFSET:", SlsComputeUnitInfo::RegionPsumOffset}};

void PrintInfo::add_subcommand(CLI::App &app) {
  sub_ = app.add_subcommand("info", "Print information about SLS ranks");
  is_hex_ = sub_->add_flag("--hex", "Print all numbers in hex");
  sub_is_initialized_ = false;

  sub_->callback([this]() { execute(); });
}

void PrintInfo::execute() {
  if (!check_sls_available()) {
    return;
  }
  enabled_ranks_.assign(pnm::device::topo().NumOfRanks, 0);
  std::iota(enabled_ranks_.begin(), enabled_ranks_.end(), 0);

  if (!sub_is_initialized_) {
    sub_is_initialized_ = true;
    sub_->add_option("-r,--ranks", enabled_ranks_, "Show info about these ranks")
        ->delimiter(',')
        ->expected(0, pnm::device::topo().NumOfRanks)
        ->check(CLI::Range(0U, pnm::device::topo().NumOfRanks - 1))
        ->default_val(enabled_ranks_);

    auto *info_types = sub_->add_option_group(
        "info_types", "What kinds of information to show (default = all)");

    for (const auto &description : descriptions_) {
      rank_info_options_.push_back(info_types->add_flag(
          description.option_name, description.option_description));
    }
  }

  const auto device = SlsDevice::make(pnm::Device::Type::SLS_AXDIMM);
  const char *format_str = *is_hex_ ? "  {:<20}{:>20x}\n" : "  {:<20} {:>20}\n";

  std::string output;
  output.reserve(2048);

  const auto print_param = [&](uint64_t rank, SlsComputeUnitInfo param,
                               const std::string &name) {
    fmt::format_to(std::back_inserter(output), fmt::runtime(format_str), name,
                   device.compute_unit_info(rank, param));
  };

  for (uint64_t rank : enabled_ranks_) {
    fmt::format_to(std::back_inserter(output), "RANK {}\n", rank);

    const bool print_all =
        std::all_of(rank_info_options_.begin(), rank_info_options_.end(),
                    [](auto *option) { return !*option; });

    for (std::size_t i = 0; i < rank_info_options_.size(); ++i) {
      if (print_all || *rank_info_options_[i]) {
        print_param(rank, descriptions_[i].compute_unit_info,
                    descriptions_[i].printable_name);
      }
    }
  }

  fmt::print("{}", output);
}

void Reset::add_subcommand(CLI::App &app) {
  CLI::App *sub = app.add_subcommand("reset-sls", "Reset SLS device");

  sub->callback([]() {
    if (!check_sls_available()) {
      return;
    }
    check_privileges();

    auto device = SlsDevice::make(pnm::Device::Type::SLS_AXDIMM);

    device.reset();

    fmt::print("SLS device is reset\n");
  });
}

void AcquisitionTimeout::add_subcommand(CLI::App &app) {
  CLI::App *sub = app.add_subcommand(
      "timeout", "Get or set SLS device acquisition timeout property");

  const std::map<std::string, uint64_t> mapping{
      {"ns", 1'000'000'000UL}, {"us", 1'000'000UL}, {"ms", 1'000UL}};
  CLI::AsNumberWithUnit time_transform(mapping);
  time_transform.description("TIME [ns, us, ms]");

  CLI::Option *set =
      sub->add_option("--set", set_timeout_, "Set acquisition-timeout to value")
          ->transform(time_transform);

  sub->callback([this, set]() {
    if (!check_sls_available()) {
      return;
    }
    auto device = SlsDevice::make(pnm::Device::Type::SLS_AXDIMM);

    if (*set) {
      check_privileges();
      device.set_acquisition_timeout(set_timeout_);
      return;
    }

    fmt::print("Acquisition time is: {} [ns]\n", device.acquisition_timeout());
  });
}

void ResourceCleanup::add_subcommand(CLI::App &app) {
  CLI::App *sub =
      app.add_subcommand("cleanup", "Get or set SLS resource cleanup");

  const std::map<std::string, bool> mapping{{OFF, false}, {ON, true}};
  CLI::CheckedTransformer on_off_transform(mapping, CLI::ignore_case);
  on_off_transform.description("[off, on]");

  CLI::Option *set = sub->add_option("--set", set_cleanup_,
                                     "Enable or disable resource cleanup")
                         ->transform(on_off_transform)
                         ->type_name("BOOL");

  sub->callback([this, set]() {
    if (!check_sls_available()) {
      return;
    }
    auto device = SlsDevice::make(pnm::Device::Type::SLS_AXDIMM);

    if (*set) {
      check_privileges();
      device.set_resource_cleanup(set_cleanup_);
      return;
    }

    fmt::print("Resouce cleanup feature is {}\n",
               device.resource_cleanup() ? ON : OFF);
  });
}

void LeakedInfo::add_subcommand(CLI::App &app) {
  CLI::App *sub = app.add_subcommand(
      "leaked",
      "Get number of processes that didn't free device resources properly");

  sub->callback([]() {
    if (!check_sls_available()) {
      return;
    }
    const auto device = SlsDevice::make(pnm::Device::Type::SLS_AXDIMM);

    uint64_t cnt = device.leaked_count();

    fmt::print("Leaked processes counter: {}\n", cnt);
  });
}

bool tools::ctl::sls::check_sls_available() {
  static bool is_unavailable = false;
  if (is_unavailable) {
    return false;
  }
  try {
    pnm::device::topo();
    return true;
  } catch (pnm::error::IO &) {
    is_unavailable = true;
    fmt::print("Warning: unable to find sls_resource module data. ");
    fmt::print("Commands for working with SLS are disabled.\n");
    return false;
  }
}
