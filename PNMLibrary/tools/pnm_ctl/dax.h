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

#ifndef TOOLS_CTL_DAX_H
#define TOOLS_CTL_DAX_H

#include "command_interface.h"

#include "CLI/App.hpp"
#include "CLI/CLI.hpp" // NOLINT(misc-include-cleaner)

namespace tools::ctl::sls {

class SetupDaxDevice final : public ICommand {
public:
  void add_subcommand(CLI::App &app) override;

private:
  void execute();
};

class DestroyDaxDevice final : public ICommand {
public:
  void add_subcommand(CLI::App &app) override;

private:
  void execute();
};

} // namespace tools::ctl::sls

#endif // TOOLS_CTL_DAX_H
