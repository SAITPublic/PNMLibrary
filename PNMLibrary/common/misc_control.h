#ifndef MISC_CONTROL_H
#define MISC_CONTROL_H

#include "common/error_message.h"
#include "common/make_error.h"

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace pnm::control {
template <typename T>
void read_file(const std::filesystem::path &path, T &value) {
  std::ifstream file(path); // NOLINT(misc-const-correctness)

  if (!file) {
    throw pnm::error::make_io(pnm::error::open_file, path.string(),
                              strerror(errno));
  }

  if (!(file >> value)) {
    throw pnm::error::make_io(pnm::error::read_from_file, path.string());
  }
}

template <typename... TArgs>
void read_file(const std::filesystem::path &path, TArgs &...values) {
  std::ifstream file(path); // NOLINT(misc-const-correctness)

  if (!file) {
    throw pnm::error::make_io(pnm::error::open_file, path.string(),
                              strerror(errno));
  }

  auto read = [&file, &path](auto &v) {
    if (!(file >> v)) {
      throw pnm::error::make_io(pnm::error::read_from_file, path.string());
    }
  };

  (read(values), ...);
}

template <typename T>
void write_file(const std::filesystem::path &path, const T &value) {
  std::ofstream file(path); // NOLINT(misc-const-correctness)

  if (!file) {
    throw pnm::error::make_io(pnm::error::open_file, path.string(),
                              strerror(errno));
  }

  if (!(file << value)) {
    throw pnm::error::make_io(pnm::error::write_to_file, path.string());
  }
}
} // namespace pnm::control

#endif // MISC_CONTROL_H
