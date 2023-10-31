#include "imdb.h"

#include "common/topology_constants.h"

#include "tools/pnm_ctl/command_interface.h"

#include "pnmlib/imdb/control.h"

#include "pnmlib/core/device.h"

#include "CLI/App.hpp"
#include "CLI/Option.hpp"
#include "CLI/Validators.hpp"

#include <fmt/core.h>

#include <cstddef>
#include <iterator>
#include <map>
#include <string>

using namespace tools::ctl::imdb;

using pnm::imdb::device::Control;
using pnm::imdb::device::topo;

void Reset::add_subcommand(CLI::App &app) {
  CLI::App *sub = app.add_subcommand("imdb-reset", "Reset the IMDB device");

  sub->add_flag("-H,--hardware", hardware_reset_,
                "In addition to software, add hardware reset")
      ->default_val(false);

  sub->callback([this]() {
    check_privileges();

    auto option = pnm::Device::ResetOptions::SoftwareOnly;

    if (hardware_reset_) {
      option = pnm::Device::ResetOptions::SoftwareHardware;
    }

    auto dev = pnm::Device::make_device(pnm::Device::Type::IMDB);
    dev->reset(option);

    fmt::print("IMDB device is reset\n");
  });
}

void PrintInfo::add_subcommand(CLI::App &app) {
  auto *sub = app.add_subcommand("imdb-info", "Get imdb information");
  sub->callback([this]() { this->exec(); });
}

void PrintInfo::exec() {
  const Control dev{};
  std::string buffer;
  buffer.reserve(2048);

  fmt::format_to(std::back_inserter(buffer), "{:<15} {:>15}\n", "Memory size",
                 dev.memory_size());

  fmt::format_to(std::back_inserter(buffer), "{:<15} {:>15}\n", "Free size",
                 dev.free_size());

  fmt::format_to(std::back_inserter(buffer), "{:<15} {:>15}\n", "Alignment",
                 dev.alignment());

  for (size_t i = 0; i < topo().NumOfThreads; ++i) {
    size_t state = dev.thread_state(i);
    fmt::format_to(std::back_inserter(buffer), "{:<15} {:>15}\n",
                   fmt::format("THREAD {} state", i), state);
  }

  fmt::print("{}", buffer);
}

void ResourceCleanup::add_subcommand(CLI::App &app) {
  auto *sub =
      app.add_subcommand("imdb-cleanup", "Control process manager's cleanup");
  const std::map<std::string, bool> mapping{{OFF, false}, {ON, true}};
  CLI::CheckedTransformer on_off_transform(mapping, CLI::ignore_case);
  on_off_transform.description("[off, on]");

  CLI::Option *set = sub->add_option("--set", set_cleanup_,
                                     "Enable or disable resource cleanup")
                         ->transform(on_off_transform)
                         ->type_name("BOOL");

  sub->callback([this, set]() {
    const Control dev{};

    if (*set) {
      check_privileges();
      dev.set_resource_cleanup(set_cleanup_);
      return;
    }

    fmt::print("Resource cleanup feature is {}\n",
               dev.get_resource_cleanup() ? ON : OFF);
  });
}

void LeakedInfo::add_subcommand(CLI::App &app) {
  CLI::App *sub = app.add_subcommand(
      "imdb-leaked",
      "Get number of processes that didn't free device resources properly");

  sub->callback([]() {
    const Control dev{};

    const auto cnt = dev.get_leaked();

    fmt::print("Leaked processes counter: {}\n", cnt);
  });
}
