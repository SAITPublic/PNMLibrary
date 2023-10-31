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

#include "dax.h"

#include "tools/pnm_ctl/command_interface.h"

#include "pnmlib/common/error.h"

#include "CLI/App.hpp"

#include <linux/sls_resources.h>

#include <ndctl/libndctl.h>
#include <unistd.h>

#include <filesystem>
#include <memory>
#include <random>
#include <utility>

using namespace tools::ctl::sls;

namespace {

const std::filesystem::path DAX_PATH_V(DAX_PATH);
const std::filesystem::path DRV_PATH_V(SLS_MEMDEV_PATH);

[[nodiscard]] bool does_symlink_exist(const std::filesystem::path &path) {
  return std::filesystem::symlink_status(path).type() ==
         std::filesystem::file_type::symlink;
}

[[nodiscard]] ndctl_ctx *create_ndctl_context() {
  ndctl_ctx *ctx;
  ndctl_new(&ctx);
  return ctx;
}

void delete_ndctl_context(ndctl_ctx *ctx) { ndctl_unref(ctx); }

} // namespace

void DestroyDaxDevice::add_subcommand(CLI::App &app) {
  CLI::App *sub =
      app.add_subcommand("destroy-dax-device", "Destroy SLS DAX device")
          ->alias("destroy-dax");

  sub->callback([this]() { execute(); });
}

using ScopedNdctlContext =
    std::unique_ptr<ndctl_ctx, decltype(&delete_ndctl_context)>;

void DestroyDaxDevice::execute() {
  check_privileges();

  const ScopedNdctlContext ctx(create_ndctl_context(),
                               std::move(delete_ndctl_context));

  auto *bus = ndctl_bus_get_first(ctx.get());

  auto *region = ndctl_region_get_first(bus);

  auto *nd_namespace = ndctl_namespace_get_first(region);
  if (!nd_namespace) {
    return;
  }

  if (auto *dax = ndctl_namespace_get_dax(nd_namespace)) {
    ndctl_dax_delete(dax);
  }

  ndctl_namespace_disable(nd_namespace);
  ndctl_namespace_set_enforce_mode(nd_namespace, NDCTL_NS_MODE_RAW);
  ndctl_namespace_delete(nd_namespace);

  ndctl_region_cleanup(region);

  if (does_symlink_exist(DRV_PATH_V)) {
    std::filesystem::remove(DRV_PATH_V);
  }
}

void SetupDaxDevice::add_subcommand(CLI::App &app) {
  CLI::App *sub = app.add_subcommand("setup-dax", "Setup SLS DAX device")
                      ->alias("setup-dax-device");

  sub->callback([this]() { execute(); });
}

void SetupDaxDevice::execute() {
  check_privileges();

  if (!std::filesystem::exists(DAX_PATH_V)) {

    const ScopedNdctlContext ctx(create_ndctl_context(),
                                 std::move(delete_ndctl_context));

    auto *bus = ndctl_bus_get_first(ctx.get());

    auto *region = ndctl_region_get_first(bus);

    auto *nd_namespace = ndctl_namespace_get_first(region);
    if (!nd_namespace) {
      nd_namespace = ndctl_region_get_namespace_seed(region);
    } else {
      ndctl_namespace_disable(nd_namespace);
    }

    auto *dax = ndctl_dax_get_first(region);
    if (!dax) {
      dax = ndctl_region_get_dax_seed(region);
    }

    std::default_random_engine gen{std::random_device{}()};
    std::uniform_int_distribution<unsigned char> distrib(0, 15);
    uuid_t uuid = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    uuid[distrib(gen)] = distrib(gen);

    ndctl_dax_set_uuid(dax, uuid);
    ndctl_dax_set_location(dax, NDCTL_PFN_LOC_RAM);
    ndctl_dax_set_namespace(dax, nd_namespace);

    auto page_size = getpagesize();
    // By default, DAX requires alignment by 2M for memory mapping. The Gramine
    // memory manager generates address with alignment by PAGE_SIZE, so
    // this leads to error during device memory mapping. Here we set DAX
    // alignment to PAGE_SIZE to avoid complex work with Gramine address
    // generation.
    ndctl_dax_set_align(dax, page_size);

    ndctl_dax_enable(dax);
    ndctl_namespace_enable(nd_namespace);
  }

  if (std::filesystem::exists(DAX_PATH_V) && !does_symlink_exist(DRV_PATH_V)) {
    std::filesystem::create_symlink(DAX_PATH_V, DRV_PATH_V);
  }

  static constexpr auto permissions =
      std::filesystem::perms::owner_read | std::filesystem::perms::owner_write |
      std::filesystem::perms::group_read | std::filesystem::perms::group_write |
      std::filesystem::perms::others_read |
      std::filesystem::perms::others_write;

  if (std::filesystem::exists(DAX_PATH_V)) {
    std::filesystem::permissions(DAX_PATH_V, permissions,
                                 std::filesystem::perm_options::replace);
    return;
  }

  throw pnm::error::IO("DAX device setup failed.");
}
