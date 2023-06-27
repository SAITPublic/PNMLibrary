#ifndef TOOLS_CTL_IMDB_H
#define TOOLS_CTL_IMDB_H

#include "command_interface.h"

#include "CLI/App.hpp"
#include "CLI/CLI.hpp" // NOLINT(misc-include-cleaner)

namespace tools::ctl::imdb {

class Reset final : public ICommand {
public:
  void add_subcommand(CLI::App &app) override;

private:
  bool hardware_reset_ = false;
};

} // namespace tools::ctl::imdb

#endif // TOOLS_CTL_IMDB_H
