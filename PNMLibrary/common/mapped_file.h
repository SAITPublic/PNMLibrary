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

#ifndef MAPPED_FILE_H
#define MAPPED_FILE_H

#include "file_descriptor_holder.h"

#include "common/error_message.h"
#include "common/log.h"
#include "common/make_error.h"

#include "pnmlib/common/views.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string_view>
#include <utility>

namespace pnm::utils {
/*! \brief Class that allows to read data via mmap */
class MappedData {
public:
  MappedData() = default;

  MappedData(int data_fd, size_t data_len, std::string_view data_kind = "file",
             size_t offset = 0, int prot = PROT_READ | PROT_WRITE,
             int flags = MAP_SHARED)
      : len_(data_len) {

    auto *data_ptr = mmap(nullptr, len_, prot, flags, data_fd, offset);
    if (data_ptr == MAP_FAILED) {
      throw pnm::error::make_io(pnm::error::mmap_common, data_kind, data_fd,
                                len_, offset, strerror(errno));
    }
    data_ptr_ = data_ptr;
  }

  MappedData(MappedData &&other) noexcept
      : len_(std::exchange(other.len_, 0)),
        data_ptr_(std::exchange(other.data_ptr_, nullptr)) {}

  MappedData &operator=(MappedData &&other) noexcept {
    unmap();
    data_ptr_ = std::exchange(other.data_ptr_, nullptr);
    len_ = std::exchange(other.len_, 0);
    return *this;
  }

  MappedData(const MappedData &other) noexcept = delete;

  MappedData &operator=(const MappedData &other) noexcept = delete;

  ~MappedData() noexcept { unmap(); }

  void *data() const noexcept { return data_ptr_; }

  // data size in bytes
  size_t size() const noexcept { return len_; }

  template <typename T> pnm::views::common<T> get_view() const & noexcept {
    return pnm::views::make_view<T>(reinterpret_cast<T *>(this->data()),
                                    reinterpret_cast<T *>(this->data()) +
                                        this->size() / sizeof(T));
  }

  template <typename T>
  pnm::views::common<T> get_view(size_t size) const & noexcept {
    assert(size <= this->size());
    return pnm::views::make_view<T>(reinterpret_cast<T *>(this->data()),
                                    reinterpret_cast<T *>(this->data()) +
                                        size / sizeof(T));
  }

  template <typename T>
  pnm::views::common<T> get_view() const && noexcept = delete;

  explicit operator bool() const noexcept { return data_ptr_ != nullptr; }

private:
  // length of mapped memory in bytes,
  size_t len_ = 0;
  void *data_ptr_{nullptr};

  void unmap() noexcept {
    if (data_ptr_ != nullptr) {
      if (munmap(data_ptr_, len_) != 0) {
        PNM_LOG_ERROR(pnm::error::munmap_memory, (uint64_t)data_ptr_, len_,
                      strerror(errno));
      }
    }
  }
};

class MappedFile {
public:
  MappedFile() = default;

  ~MappedFile() = default;

  explicit MappedFile(const std::filesystem::path &data_path,
                      std::string_view data_kind = "file")
      : fd_(data_path.string(), O_RDONLY),
        map_data_(*fd_, lseek(*fd_, 0, SEEK_END), data_kind, 0, PROT_READ,
                  MAP_PRIVATE) {}

  MappedFile(const std::filesystem::path &data_path, size_t len,
             std::string_view data_kind = "file", size_t offset = 0,
             int prot = PROT_READ | PROT_WRITE, int flags = MAP_SHARED)
      : fd_(data_path.string(), O_RDWR),
        map_data_(*fd_, len, data_kind, offset, prot, flags) {}

  MappedFile &operator=(MappedFile &&other) noexcept = default;

  MappedFile(MappedFile &&other) noexcept = default;

  void *data() const noexcept { return map_data_.data(); }

  // data size in bytes
  size_t size() const noexcept { return map_data_.size(); }

  template <typename T> pnm::views::common<T> get_view() const & noexcept {
    return map_data_.get_view<T>();
  }

  template <typename T>
  pnm::views::common<T> get_view() const && noexcept = delete;

  explicit operator bool() const noexcept { return bool(map_data_); }

private:
  FileDescriptorHolder fd_;
  MappedData map_data_;
};

} // namespace pnm::utils
#endif // MAPPED_FILE_H
