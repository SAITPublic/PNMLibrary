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

#ifndef _PNM_ACCESSOR_BUILDER_H_
#define _PNM_ACCESSOR_BUILDER_H_

#include "accessor.h"
#include "context.h"
#include "memory.h"

#include "pnmlib/common/compiler.h"

#include <memory>

namespace pnm::memory {

PNM_API std::unique_ptr<AccessorCore>
create_accessor_core(const DeviceRegion &region, Context *ctx);

} // namespace pnm::memory

#endif //_PNM_ACCESSOR_BUILDER_H_
