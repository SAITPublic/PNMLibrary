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

#include "shared.h"

#include "common/make_error.h"

#include "tools/pnm_ctl/command_interface.h"

#include "pnmlib/sls/control.h"

#include "pnmlib/imdb/control.h"
#include "pnmlib/imdb/libimdb.h"

#include "pnmlib/common/error.h"

#include "CLI/App.hpp"
#include "CLI/Option.hpp"
#include "CLI/Validators.hpp"

#include <linux/imdb_resources.h>
#include <linux/sls_resources.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>

using namespace tools::ctl;

void DestroySharedMemory::add_subcommand(CLI::App &app) {
  CLI::App *sub = app.add_subcommand(
      "destroy-shm", "Destroy shared memory files for simulator mode");

  auto *device_type =
      sub->add_option_group("dev_type", "Device type")->required();

  device_type->add_flag("-s,--sls", is_sls_,
                        "Destroy memory for SLS. i.e. one region "
                        "for both memory and registers");

  device_type->add_flag("-i,--imdb", is_imdb_,
                        "Destroy memory for IMDB. i.e. two regions - "
                        "one for memory and one for registers");

  sub->callback([this]() { execute(); });
}

void DestroySharedMemory::destroy_shm(const char *name) {
  if (shm_unlink(name) == -1) {
    if (errno == ENOENT) {
      print_err("Shared memory file {} doesn't exist, skipping!\n", name);
      return;
    }

    throw pnm::error::make_io(
        "Unable to unlink shared memory file. Name: {}. Reason: {}.", name,
        strerror(errno));
  }
}

void DestroySharedMemory::execute() {
  if (is_sls_) {
    destroy_shm(SLS_SHMEM);
  }
  if (is_imdb_) {
    destroy_shm(IMDB_CSR_DEVICE_SIM_NAME);
    destroy_shm(IMDB_DEVMEMORY_PATH_SIM_NAME);
  }
  if (!is_sls_ && !is_imdb_) {
    throw pnm::error::InvalidArguments("Bad state.");
  }
}

void SetupSharedMemory::add_subcommand(CLI::App &app) {
  CLI::App *sub = app.add_subcommand(
      "setup-shm", "Setup shared memory files for simulator mode");

  auto *device_type =
      sub->add_option_group("dev_type", "Device type")->required();

  device_type->add_flag("-s,--sls", is_sls_,
                        "Setup memory for SLS. i.e. one region "
                        "for both memory and registers");

  CLI::Option *imdb =
      device_type->add_flag("-i,--imdb", is_imdb_,
                            "Setup memory for IMDB. i.e. two regions - "
                            "one for memory and one for registers");

  sub->add_option("-r,--regs", regs_size_, "Device registers size")
      ->transform(CLI::AsSizeValue(false))
      ->default_val("0x1900B")
      ->needs(imdb);

  sub->add_option("-m,--mem", mem_size_,
                  "Device memory size. Cannot be used when initializing "
                  "shm both sls and imdb at the same time")
      ->transform(CLI::AsSizeValue(false));

  sub->callback([this]() { execute(); });
}

void SetupSharedMemory::execute() {
  if (is_sls_ && is_imdb_ && (mem_size_ != 0 || regs_size_ != REG_AREA_SIZE)) {
    throw pnm::error::InvalidArguments(
        "The use of options in case of setup of sls and imdb at the same time "
        "is prohibited.");
  }

  if (is_sls_) {
    if (mem_size_ == 0) {
      if (!std::filesystem::exists(SLS_SYSFS_ROOT)) {
        throw pnm::error::InvalidArguments(
            "Memory size should be specified if module not loaded");
      }

      mem_size_ = pnm::sls::device::Control{}.memory_size();
    }

    make_shm(SLS_SHMEM, mem_size_);
  }

  if (is_imdb_) {
    if (mem_size_ == 0) {
      if (!std::filesystem::exists(IMDB_SYSFS_PATH)) {
        throw pnm::error::InvalidArguments(
            "Memory size should be specified if module not loaded");
      }

      mem_size_ = pnm::imdb::device::Control{}.memory_size();
    }

    make_shm(IMDB_CSR_DEVICE_SIM_NAME, regs_size_);
    make_shm(IMDB_DEVMEMORY_PATH_SIM_NAME, mem_size_);
  }

  if (!is_sls_ && !is_imdb_) {
    throw pnm::error::InvalidArguments("Bad state.");
  }
}

void SetupSharedMemory::make_shm(const char *name, uint64_t size) {
  constexpr int O_RDWR_UNIQ = O_RDWR | O_CREAT | O_EXCL;
  constexpr int S_IRWALL =
      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

  const int fd = shm_open(name, O_RDWR_UNIQ, S_IRUSR | S_IWUSR);

  if (fd == -1) {
    if (errno == EEXIST) {
      print_err("Shared memory file {} already exists, skipping!\n", name);
      return;
    }

    throw pnm::error::make_io(
        "Unable to open shared memory file. Name: {}. Reason: {}.", name,
        strerror(errno));
  }

  if (ftruncate(fd, size) == -1) {
    throw pnm::error::make_io(
        "Unable to resize shared memory file. Name: {}. Size: {}. Reason: {}.",
        name, size, strerror(errno));
  }

  if (fchmod(fd, S_IRWALL) == -1) {
    throw pnm::error::make_io(
        "Unable to change shared memory file mode. Name: {}. Reason: {}.", name,
        strerror(errno));
  }

  if (close(fd) == -1) {
    throw pnm::error::make_io(
        "Unable to close shared memory file. Name: {}. Reason: {}.", name,
        strerror(errno));
  }
}
