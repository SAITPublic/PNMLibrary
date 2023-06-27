#include "imdb.h"

#include "tools/pnm_ctl/command_interface.h"

#include "pnmlib/core/device.h"

#include "CLI/App.hpp"

#include <fmt/core.h>

using namespace tools::ctl::imdb;

void Reset::add_subcommand(CLI::App &app) {
  CLI::App *sub = app.add_subcommand("reset-imdb", "Reset the IMDB device");

  sub->add_flag("-H,--hardware", hardware_reset_,
                "In addition to software, add hardware reset")
      ->default_val(false);

  sub->callback([this]() {
    check_privileges();

    auto option = pnm::Device::ResetOptions::SoftwareOnly;

    if (hardware_reset_) {
      option = pnm::Device::ResetOptions::SoftwareHardware;
    }

    auto dev = pnm::Device::make_device(pnm::Device::Type::IMDB_CXL);
    dev->reset(option);

    fmt::print("IMDB device is reset\n");
  });
}
