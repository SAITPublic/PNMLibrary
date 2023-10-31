#ifndef TOOLS_CTL_IMDB_H
#define TOOLS_CTL_IMDB_H

#include "command_interface.h"

#include "CLI/App.hpp"
#include "CLI/CLI.hpp" // NOLINT(misc-include-cleaner)

#include <cstdint>

namespace tools::ctl::imdb {

class Reset : public ICommand {
public:
  void add_subcommand(CLI::App &app) override;

private:
  bool hardware_reset_ = false;
};

class PrintInfo : public ICommand {
public:
  void add_subcommand(CLI::App &app) override;

private:
  void exec();
};

class ResourceCleanup : public ICommand {
public:
  void add_subcommand(CLI::App &app) override;

private:
  static constexpr auto ON = "on";
  static constexpr auto OFF = "off";

  uint64_t set_cleanup_{};
};

class LeakedInfo : public ICommand {
public:
  void add_subcommand(CLI::App &app) override;
};

} // namespace tools::ctl::imdb

#endif // TOOLS_CTL_IMDB_H
