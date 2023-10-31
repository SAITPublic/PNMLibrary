// core/device/imdb/hardware.h - imdb HW device ops -*- C++ -*-//

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

#ifndef _IMDB_HARDWARE_DEVICE_H_
#define _IMDB_HARDWARE_DEVICE_H_

#include "base.h"

namespace pnm::imdb::device {

class HardwareDevice : public BaseDevice {
public:
  HardwareDevice();
  ~HardwareDevice() override;
};

} // namespace pnm::imdb::device

#endif /* _IMDB_HARDWARE_DEVICE_H_ */
