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

#ifndef _PNM_ALLOCATOR_H_
#define _PNM_ALLOCATOR_H_

#include "memory.h"

#include "pnmlib/common/compiler.h"
#include "pnmlib/common/properties.h"

#include <cstdint>

namespace pnm::memory {
/** @brief Base class for PNM memory allocator */
class Allocator {
public:
  DeviceRegion allocate(uint64_t size,
                        const property::PropertiesList &props = {}) {
    return allocate_impl(size, props);
  }

  void deallocate(const DeviceRegion &region) { deallocate_impl(region); }

  virtual ~Allocator() = default;

private:
  virtual DeviceRegion allocate_impl(uint64_t size,
                                     const property::PropertiesList &props) = 0;

  virtual void deallocate_impl(const DeviceRegion &region) = 0;
};

namespace property {
/** @brief Specifies the number of the computational block whose memory will be
 * used for data storage. */
struct PNM_API CURegion
    : pnm::property::Base<pnm::property::PropertiesTypes::MEM_LOCATION> {
  explicit CURegion(uint32_t rank) : rank_{rank} {}
  auto rank() const { return rank_; }

private:
  uint32_t rank_;
};

struct PNM_API AllocPolicy
    : pnm::property::Base<
          pnm::property::PropertiesTypes::MEM_ALLOCATION_POLICY> {
  explicit AllocPolicy(uint32_t policy) : policy_{policy} {}
  auto policy() const { return policy_; }

private:
  uint32_t policy_;
};

} // namespace property
} // namespace pnm::memory

#endif //_PNM_ALLOCATOR_H_
