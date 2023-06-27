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

#include "pnmlib/imdb/libimdb.h"

#include "pnmlib/common/error.h"

#include "CLI/App.hpp"
#include "CLI/Option.hpp"
#include "CLI/Validators.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>

using namespace tools::ctl;

void DestroySharedMemory::add_subcommand(CLI::App &app) {
  CLI::App *sub = app.add_subcommand(
      "destroy-shm", "Destroy shared memory files for simulator mode");

  auto *device_type =
      sub->add_option_group("dev_type", "Device type")->required();

  CLI::Option *sls =
      device_type->add_flag("-s,--sls", is_sls_,
                            "Destroy memory for SLS. i.e. one region "
                            "for both memory and registers");
  device_type
      ->add_flag("-i,--imdb", is_imdb_,
                 "Destroy memory for IMDB. i.e. two regions - "
                 "one for memory and one for registers")
      ->excludes(sls);

  sub->callback([this]() { execute(); });
}

void DestroySharedMemory::destroy_shm(const char *name) {
  if (shm_unlink(name) == -1) {
    if (errno == ENOENT) {
      print_err("Shared memory file {} doesn't exist, skpping!\n", name);
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
  } else if (is_imdb_) {
    destroy_shm(IMDB_CSR_DEVICE_SIM_NAME);
    destroy_shm(IMDB_DEVMEMORY_PATH_SIM_NAME);
  } else {
    throw pnm::error::InvalidArguments("Bad state.");
  }
}

void SetupSharedMemory::add_subcommand(CLI::App &app) {
  CLI::App *sub = app.add_subcommand(
      "setup-shm", "Setup shared memory files for simulator mode");

  auto *device_type =
      sub->add_option_group("dev_type", "Device type")->required();

  CLI::Option *sls =
      device_type->add_flag("-s,--sls", is_sls_,
                            "Setup memory for SLS. i.e. one region "
                            "for both memory and registers");
  CLI::Option *imdb =
      device_type
          ->add_flag("-i,--imdb", is_imdb_,
                     "Setup memory for IMDB. i.e. two regions - "
                     "one for memory and one for registers")
          ->excludes(sls);

  sub->add_option("-m,--mem", mem_size_, "Device memory size")
      ->transform(CLI::AsSizeValue(false))
      ->default_val("16GB");

  sub->add_option("-r,--regs", regs_size_, "Device registers size")
      ->transform(CLI::AsSizeValue(false))
      ->default_val("0x1900B")
      ->needs(imdb);

  sub->callback([this]() { execute(); });
}

void SetupSharedMemory::execute() {
  if (is_sls_) {
    make_shm(SLS_SHMEM, mem_size_);
  } else if (is_imdb_) {
    make_shm(IMDB_CSR_DEVICE_SIM_NAME, regs_size_);
    make_shm(IMDB_DEVMEMORY_PATH_SIM_NAME, mem_size_);
  } else {
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
