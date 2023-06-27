/*
 *  Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 *  This software is proprietary of Samsung Electronics.
 *  No part of this software, either material or conceptual may be copied or
 *  distributed, transmitted, transcribed, stored in a retrieval system or
 *  translated into any human or computer language in any form by any means,
 *  electronic, mechanical, manual or otherwise, or disclosed to third parties
 *  without the express written permission of Samsung Electronics.
 */

#include "pnmlib/sls/embedded_tables.h"

#include "common/timer.h"
#include "common/topology_constants.h"

#include "pnmlib/core/allocator.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/memory.h"
#include "pnmlib/core/memory_object.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/properties.h"
#include "pnmlib/common/views.h"

#include <linux/sls_resources.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>

pnm::memory::EmbeddedTables::EmbeddedTables(
    const std::vector<uint32_t> &table_sizes, uint64_t row_size,
    const pnm::ContextHandler &ctx, sls_user_preferences option) {
  init_buffers(table_sizes, row_size, ctx, option);
}

void pnm::memory::EmbeddedTables::init_buffers(
    const std::vector<uint32_t> &table_sizes, uint64_t row_bytes,
    const pnm::ContextHandler &ctx, sls_user_preferences option) {
  MEASURE_TIME();

  pnm::property::PropertiesList props;
  if (option == SLS_ALLOC_REPLICATE_ALL || option == SLS_ALLOC_AUTO) {
    props.add_property(property::AllocPolicy(SLS_ALLOC_REPLICATE_ALL));
  } else if (option != SLS_ALLOC_DISTRIBUTE_ALL) {
    throw pnm::error::InvalidArguments("Unknown memory policy.");
  }

  for (auto size : table_sizes) {
    buffers_.emplace_back(size * row_bytes, ctx, props);
  }

  construct_layout();
}

void pnm::memory::EmbeddedTables::transfer_data(
    pnm::common_view<const uint8_t> host) {
  const auto *begin = host.begin();
  for (auto &buffer : buffers_) {
    auto host_data = pnm::make_view(begin, buffer.size());
    buffer.bind_user_region(pnm::view_const_cast<uint8_t>(host_data));
    buffer.copy_to_device();
    begin = host_data.end();
  }
}

void pnm::memory::EmbeddedTables::bind_tables_data(
    pnm::common_view<const uint8_t> host) {
  transfer_data(host);
}

void pnm::memory::EmbeddedTables::construct_layout() {
  layout_.resize(1UL << pnm::device::topo().NumOfRanks);

  auto table_id = 0UL;
  for (const auto &buffer : buffers()) {
    auto address = std::get<pnm::memory::RankedRegion>(buffer.device_region());

    auto valid_region =
        std::find_if(address.regions.begin(), address.regions.end(),
                     [](const auto &e) { return e.location != -1; });

    assert(valid_region != address.regions.end());

    auto mask = 0U;
    MemoryObject mem_object{table_id, valid_region->size};
    for (const auto &subaddr : address.regions) {
      if (subaddr.location != -1) {
        mask |= (1 << subaddr.location);
        mem_object.set_offset(subaddr.location, subaddr.start);
      }
    }

    layout_[mask].emplace_back(std::move(mem_object));

    ++table_id;
  }
}
