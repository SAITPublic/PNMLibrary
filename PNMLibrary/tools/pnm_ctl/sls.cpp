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

#include "pnmlib/sls/control.h"

#include "pnmlib/core/device.h"
#include "pnmlib/core/sls_cunit_info.h"

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
using pnm::sls::ComputeUnitInfo;
using pnm::sls::device::Control;
using pnm::sls::device::topo;

// [TODO: MCS23-1505] rework this array
const std::array<details::RankInfoDescription, pnm::sls::cunit_info_length>
    PrintInfo::descriptions_ = {
        details::RankInfoDescription{
            "--state",
            "Show state of rank (0 = free, 1 = busy)",
            "STATE:",
            ComputeUnitInfo::State,
        },
        details::RankInfoDescription{
            "--acquisition-count",
            "Show acquisition count of rank",
            "ACQUISITION COUNT:",
            ComputeUnitInfo::AcquisitionCount,
        },
        details::RankInfoDescription{
            "--free-size",
            "Show size of free rank memory in bytes",
            "FREE SIZE:",
            ComputeUnitInfo::FreeSize,
        },
        details::RankInfoDescription{
            "--base-size",
            "Show size of BASE region in bytes",
            "BASE SIZE:",
            ComputeUnitInfo::RegionBaseSize,
        },
        details::RankInfoDescription{
            "--base-offset",
            "Show offset of BASE region in bytes",
            "BASE OFFSET:",
            ComputeUnitInfo::RegionBaseOffset,
        },
        details::RankInfoDescription{
            "--base-map-size",
            "Show size of mapped BASE region in bytes",
            "BASE MAP SIZE:",
            ComputeUnitInfo::RegionBaseMapSize,
        },
        details::RankInfoDescription{
            "--base-map-offset",
            "Show offset of mapped BASE region in bytes",
            "BASE MAP OFFSET:",
            ComputeUnitInfo::RegionBaseMapOffset,
        },
        details::RankInfoDescription{
            "--inst-size",
            "Show size of INST region in bytes",
            "INST SIZE:",
            ComputeUnitInfo::RegionInstSize,
        },
        details::RankInfoDescription{
            "--inst-offset",
            "Show offset of INST region in bytes",
            "INST OFFSET:",
            ComputeUnitInfo::RegionInstOffset,
        },

        details::RankInfoDescription{
            "--inst-map-size",
            "Show size of mapped INST region in bytes",
            "INST MAP SIZE:",
            ComputeUnitInfo::RegionInstMapSize,
        },
        details::RankInfoDescription{
            "--inst-map-offset",
            "Show offset of mapped INST region in bytes",
            "INST MAP OFFSET:",
            ComputeUnitInfo::RegionInstMapOffset,
        },
        details::RankInfoDescription{
            "--cfgr-size",
            "Show size of CFGR region in bytes",
            "CFGR SIZE:",
            ComputeUnitInfo::RegionCfgrSize,
        },
        details::RankInfoDescription{
            "--cfgr-offset",
            "Show offset of CFGR region in bytes",
            "CFGR OFFSET:",
            ComputeUnitInfo::RegionCfgrOffset,
        },
        details::RankInfoDescription{
            "--cfgr-map-size",
            "Show size of mapped CFGR region in bytes",
            "CFGR MAP SIZE:",
            ComputeUnitInfo::RegionCfgrMapSize,
        },
        details::RankInfoDescription{
            "--cfgr-map-offset",
            "Show offset of mapped CFGR region in bytes",
            "CFGR MAP OFFSET:",
            ComputeUnitInfo::RegionCfgrMapOffset,
        },
        details::RankInfoDescription{
            "--tags-size",
            "Show size of TAGS region in bytes",
            "TAGS SIZE:",
            ComputeUnitInfo::RegionTagsSize,
        },
        details::RankInfoDescription{
            "--tags-offset",
            "Show offset of TAGS region in bytes",
            "TAGS OFFSET:",
            ComputeUnitInfo::RegionTagsOffset,
        },
        details::RankInfoDescription{
            "--tags-map-size",
            "Show size of mapped TAGS region in bytes",
            "TAGS MAP SIZE:",
            ComputeUnitInfo::RegionTagsMapSize,
        },
        details::RankInfoDescription{
            "--tags-map-offset",
            "Show offset of mapped TAGS region in bytes",
            "TAGS MAP OFFSET:",
            ComputeUnitInfo::RegionTagsMapOffset,
        },
        details::RankInfoDescription{
            "--psum-size",
            "Show size of PSUM region in bytes",
            "PSUM SIZE:",
            ComputeUnitInfo::RegionPsumSize,
        },
        details::RankInfoDescription{
            "--psum-offset",
            "Show offset of PSUM region in bytes",
            "PSUM OFFSET:",
            ComputeUnitInfo::RegionPsumOffset,
        },
        details::RankInfoDescription{
            "--psum-map-size",
            "Show size of mapped PSUM region in bytes",
            "PSUM MAP SIZE:",
            ComputeUnitInfo::RegionPsumMapSize,
        },
        details::RankInfoDescription{
            "--psum-map-offset",
            "Show offset of mapped PSUM region in bytes",
            "PSUM MAP OFFSET:",
            ComputeUnitInfo::RegionPsumMapOffset,
        },
};

void PrintInfo::add_subcommand(CLI::App &app) {
  CLI::App *sub =
      app.add_subcommand("sls-info", "Print information about SLS ranks");

  is_hex_ = sub->add_flag("--hex", "Print all numbers in hex");

  enabled_ranks_.assign(topo().NumOfCUnits, 0);
  std::iota(enabled_ranks_.begin(), enabled_ranks_.end(), 0);

  sub->add_option("-r,--ranks", enabled_ranks_, "Show info about these ranks")
      ->delimiter(',')
      ->expected(0, topo().NumOfCUnits)
      ->check(CLI::Range(0U, topo().NumOfCUnits - 1))
      ->default_val(enabled_ranks_);

  auto *info_types = sub->add_option_group(
      "info_types", "What kinds of information to show (default = all)");

  for (const auto &description : descriptions_) {
    rank_info_options_.push_back(info_types->add_flag(
        description.option_name, description.option_description));
  }

  sub->callback([this]() { execute(); });
}

void PrintInfo::execute() {
  const Control control{};

  const char *format_str = *is_hex_ ? "  {:<20}{:>20x}\n" : "  {:<20} {:>20}\n";

  std::string output;
  output.reserve(2048);

  const auto print_param = [&](uint64_t rank, ComputeUnitInfo param,
                               const std::string &name) {
    fmt::format_to(std::back_inserter(output), fmt::runtime(format_str), name,
                   control.get_compute_unit_info(rank, param));
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
  CLI::App *sub = app.add_subcommand("sls-reset", "Reset SLS device");

  sub->callback([]() {
    check_privileges();

    auto device = pnm::Device::make_device(pnm::Device::Type::SLS);

    device->reset();

    fmt::print("SLS device is reset\n");
  });
}

void AcquisitionTimeout::add_subcommand(CLI::App &app) {
  CLI::App *sub = app.add_subcommand(
      "sls-timeout", "Get or set SLS device acquisition timeout property");

  const std::map<std::string, uint64_t> mapping{
      {"ns", 1'000'000'000UL}, {"us", 1'000'000UL}, {"ms", 1'000UL}};
  CLI::AsNumberWithUnit time_transform(mapping);
  time_transform.description("TIME [ns, us, ms]");

  CLI::Option *set =
      sub->add_option("--set", set_timeout_, "Set acquisition-timeout to value")
          ->transform(time_transform);

  sub->callback([this, set]() {
    const Control control{};

    if (*set) {
      check_privileges();
      control.set_acquisition_timeout(set_timeout_);
      return;
    }

    fmt::print("Acquisition time is: {} [ns]\n",
               control.get_acquisition_timeout());
  });
}

void ResourceCleanup::add_subcommand(CLI::App &app) {
  CLI::App *sub =
      app.add_subcommand("sls-cleanup", "Get or set SLS resource cleanup");

  const std::map<std::string, bool> mapping{{OFF, false}, {ON, true}};
  CLI::CheckedTransformer on_off_transform(mapping, CLI::ignore_case);
  on_off_transform.description("[off, on]");

  CLI::Option *set = sub->add_option("--set", set_cleanup_,
                                     "Enable or disable resource cleanup")
                         ->transform(on_off_transform)
                         ->type_name("BOOL");

  sub->callback([this, set]() {
    const Control control{};

    if (*set) {
      check_privileges();
      control.set_resource_cleanup(set_cleanup_);
      return;
    }

    fmt::print("Resouce cleanup feature is {}\n",
               control.get_resource_cleanup() ? ON : OFF);
  });
}

void LeakedInfo::add_subcommand(CLI::App &app) {
  CLI::App *sub = app.add_subcommand(
      "sls-leaked",
      "Get number of processes that didn't free device resources properly");

  sub->callback([]() {
    const Control control{};

    uint64_t cnt = control.get_leaked();

    fmt::print("Leaked processes counter: {}\n", cnt);
  });
}
