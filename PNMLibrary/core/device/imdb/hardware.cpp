// hardware.cpp - imdb hardware specific device ops --------------//

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

#include "hardware.h"

#include "core/device/imdb/base.h"

#include "common/log.h"

#include "pnmlib/imdb/libimdb.h"

namespace pnm::imdb::device {

HardwareDevice::HardwareDevice()
    : BaseDevice(IMDB_CSR_DEVICE, IMDB_DEVMEMORY_PATH) {
  pnm::log::info("IMDB HardwareDevice is initialized.");
}

HardwareDevice::~HardwareDevice() {
  pnm::log::info("IMDB HardwareDevice is deinitialized.");
}

} // namespace pnm::imdb::device
