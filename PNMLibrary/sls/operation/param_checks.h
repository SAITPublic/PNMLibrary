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

#ifndef _SLS_PARAM_CHECKS_H_
#define _SLS_PARAM_CHECKS_H_

#include "core/device/sls/base.h"

#include "pnmlib/sls/operation.h"

#include "linux/sls_resources.h"

#include <cstddef>
#include <cstdint>

namespace pnm::sls {

void check_sparse_feature_size(uint32_t sparse_feature_size);
void check_device_memory_limits(const device::BaseDevice &device,
                                size_t objects_size_in_bytes,
                                sls_user_preferences preference);
void check_run_params(const pnm::operations::SlsOperation &op);

} // namespace pnm::sls
#endif
