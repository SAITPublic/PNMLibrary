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

#include "command_interface.h"
#include "dax.h"
#include "imdb.h"
#include "shared.h"
#include "sls.h"
#include "spdlog/common.h"

#include "common/log.h"

#include "CLI/App.hpp"
#include "CLI/CLI.hpp" // NOLINT(misc-include-cleaner)
#include "CLI/Error.hpp"

#include <exception>
#include <memory>
#include <vector>

using namespace tools::ctl;

auto add_subcommands(CLI::App &app) {
  std::vector<std::unique_ptr<ICommand>> commands;
  commands.emplace_back(std::make_unique<SetupSharedMemory>());
  commands.emplace_back(std::make_unique<DestroySharedMemory>());
  commands.emplace_back(std::make_unique<imdb::Reset>());
  commands.emplace_back(std::make_unique<sls::SetupDaxDevice>());
  commands.emplace_back(std::make_unique<sls::DestroyDaxDevice>());
  commands.emplace_back(std::make_unique<sls::Reset>());
  commands.emplace_back(std::make_unique<sls::PrintInfo>());
  commands.emplace_back(std::make_unique<sls::AcquisitionTimeout>());
  commands.emplace_back(std::make_unique<sls::ResourceCleanup>());
  commands.emplace_back(std::make_unique<sls::LeakedInfo>());

  for (auto &command : commands) {
    command->add_subcommand(app);
  }

  return commands;
}

int main(int argc, char **argv) {
  CLI::App app{"Utility to control PNM devices"};

  auto commands = add_subcommands(app);

  // NOTE: Exactly one required - no point in zero. If more than one, it also
  // doesn't make sense - e.g. `pnm_ctl reset-sls reset-sls` possible if
  // we don't ban it
  app.require_subcommand(1);

  pnm::log::set_active_level(pnm::log::level::err);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    return app.exit(e);
  } catch (std::exception &e) {
    print_err("Got error: {}\n", e.what());
    return 1;
  }

  return 0;
}
