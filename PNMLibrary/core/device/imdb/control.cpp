/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#include "control.h"

#include "common/make_error.h"
#include "common/misc_control.h"

#include <fmt/core.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>

namespace pnm::imdb::device {

size_t Control::memory_size() const {
  const auto path = sysfs_root_ / "mem_size";
  size_t value;
  pnm::control::utils::read_file(path, value);

  return value;
}

size_t Control::alignment() const {
  const auto path = sysfs_root_ / "alignment";
  size_t value;
  pnm::control::utils::read_file(path, value);

  return value;
}

size_t Control::free_size() const {
  const auto path = sysfs_root_ / "free_size";
  size_t value;
  pnm::control::utils::read_file(path, value);

  return value;
}

bool Control::thread_state(uint8_t thread) const {
  const auto path = sysfs_root_ / fmt::format("{}", thread) / "state";
  size_t value;
  pnm::control::utils::read_file(path, value);

  return value;
}

void Control::reset() {
  const auto path = sysfs_root_ / "reset";
  pnm::control::utils::write_file(path, 1);
}

Control::Control() {
  if (!std::filesystem::exists(sysfs_root_)) {
    throw pnm::error::make_io("Module not loaded.");
  }
}

bool Control::get_resource_cleanup() const {
  const auto path = sysfs_root_ / "cleanup";
  size_t value;
  pnm::control::utils::read_file(path, value);
  return value == 1;
}

uint64_t Control::get_leaked() const {
  const auto path = sysfs_root_ / "leaked";
  size_t value;
  pnm::control::utils::read_file(path, value);
  return value;
}

void Control::set_resource_cleanup(bool state) const {
  const auto path = sysfs_root_ / "cleanup";
  pnm::control::utils::write_file(path, state);
}

} // namespace pnm::imdb::device
