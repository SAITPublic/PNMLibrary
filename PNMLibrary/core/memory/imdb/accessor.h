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

#ifndef _IMDB_ACCESSOR_H_
#define _IMDB_ACCESSOR_H_

#include "core/device/imdb/base.h"
#include "core/memory/sequential_accessor.h"

#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

namespace pnm::memory {
class ImdbAccessorCore : public SequentialAccessorCore {
public:
  ImdbAccessorCore(const pnm::memory::DeviceRegion &region,
                   DevicePointer &device)
      : SequentialAccessorCore(
            region, device,
            device->as<pnm::imdb::device::BaseDevice>()->alignment()) {}
};
} // namespace pnm::memory

#endif //_IMDB_ACCESSOR_H_
