/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 */

#ifndef FILE_DESCRIPTOR_HOLDER_H
#define FILE_DESCRIPTOR_HOLDER_H

#include "common/error_message.h"
#include "common/make_error.h"

#include "pnmlib/common/error.h"

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>
#include <utility>

namespace pnm::utils {

/*! \brief RAII-style wrapper for file descriptors */
class FileDescriptorHolder {
public:
  FileDescriptorHolder() = default;

  FileDescriptorHolder(const std::string &pathname, int flags,
                       mode_t mode = 0) {
    fd_ = open(pathname.c_str(), flags, mode);
    if (!is_valid()) {
      throw pnm::error::make_io(pnm::error::open_file, pathname,
                                strerror(errno));
    }
  }

  FileDescriptorHolder(const FileDescriptorHolder &other) = delete;

  FileDescriptorHolder &operator=(const FileDescriptorHolder &other) = delete;

  FileDescriptorHolder(FileDescriptorHolder &&other) noexcept
      : fd_(std::exchange(other.fd_, -1)) {}

  FileDescriptorHolder &operator=(FileDescriptorHolder &&other) noexcept {
    close();
    fd_ = std::exchange(other.fd_, -1);
    return *this;
  }

  ~FileDescriptorHolder() noexcept { close(); }

  // return fd_ if it is valid file descriptor, throws otherwise
  int get() const & {
    if (!is_valid()) {
      throw pnm::error::IO("Invalid file descriptor.");
    }
    return fd_;
  }

  int operator*() const & { return get(); }

  bool is_valid() const noexcept {
    return fcntl(fd_, F_GETFD) != -1 || errno != EBADF;
  }

  operator bool() const noexcept { return is_valid(); }

private:
  int fd_ = -1;

  void close() noexcept {
    if (is_valid()) {
      ::close(fd_);
    }
    fd_ = -1;
  }
};

} // namespace pnm::utils
#endif // FILE_DESCRIPTOR_HOLDER_H
