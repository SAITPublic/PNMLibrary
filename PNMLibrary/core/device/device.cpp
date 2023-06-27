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

#include "pnmlib/core/device.h"

#include "core/device/imdb/base.h"
#include "core/device/sls/base.h"

#include "common/make_error.h"

pnm::DevicePointer pnm::Device::make_device(Type type) {
  switch (type) {
  case Type::SLS_AXDIMM:
  case Type::SLS_CXL:
    return pnm::sls::device::BaseDevice::make_device(type);
  case Type::IMDB_CXL:
    return pnm::imdb::device::BaseDevice::make_device();
  }
  throw pnm::error::make_inval("Unknown device type {}.",
                               static_cast<int>(type));
}
