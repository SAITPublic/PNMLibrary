/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 */

#ifndef PNM_TEMPORARY_FILESYSTEM_OBJECT_H
#define PNM_TEMPORARY_FILESYSTEM_OBJECT_H

#include "error_message.h"
#include "make_error.h"

#include <stdlib.h> // NOLINT(modernize-deprecated-headers, misc-include-cleaner)
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <string>

namespace pnm::utils {

class TemporaryFilesystemObject {
public:
  TemporaryFilesystemObject()
      : object_name{
            (std::filesystem::temp_directory_path() / "tmpXXXXXX").string()} {}

  const std::string &name() const {
    if (exists()) {
      return object_name;
    }
    throw pnm::error::make_io("Filesystem object {} does not exist.",
                              object_name);
  }

  bool exists() const { return std::filesystem::exists(object_name); }

  operator bool() const { return exists(); }

  virtual ~TemporaryFilesystemObject() {
    std::filesystem::remove_all(object_name);
  }

protected:
  std::string object_name;
};

/** @brief RAII-wrapper over temporary file
 */
class TemporaryFile : public TemporaryFilesystemObject {
public:
  TemporaryFile() {
    const int temp_fd = mkstemp(object_name.data());
    if (temp_fd == -1) {
      throw pnm::error::make_io(pnm::error::open_common, "temporary file",
                                strerror(errno));
    }
    if (close(temp_fd) == -1) {
      throw pnm::error::make_io(pnm::error::close_common,
                                "temporary file descriptor", strerror(errno));
    }
  }
};

/** @brief RAII-wrapper over temporary directory
 */
class TemporaryDirectory : public TemporaryFilesystemObject {
public:
  TemporaryDirectory() {
    char *res = mkdtemp(object_name.data());
    if (res == nullptr) {
      throw pnm::error::make_io(pnm::error::open_common, "temporary directory",
                                strerror(errno));
    }
  }
};

} // namespace pnm::utils

#endif // PNM_TEMPORARY_FILESYSTEM_OBJECT_H
