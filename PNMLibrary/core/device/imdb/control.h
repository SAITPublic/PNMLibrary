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

#ifndef CONTROL_H
#define CONTROL_H

#include <linux/imdb_resources.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>

namespace pnm::imdb::device {

class Control {
public:
  Control();

  size_t memory_size() const;

  size_t free_size() const;

  size_t alignment() const;

  bool thread_state(uint8_t thread) const;

  void reset();

  uint64_t get_leaked() const;

  bool get_resource_cleanup() const;

  void set_resource_cleanup(bool state) const;

private:
  std::filesystem::path sysfs_root_ = IMDB_SYSFS_PATH;
};

} // namespace pnm::imdb::device
#endif // CONTROL_H
