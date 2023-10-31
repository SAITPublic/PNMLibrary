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

#ifndef _SLS_CXL_HW_MEMBLOCKHANDLER_H_
#define _SLS_CXL_HW_MEMBLOCKHANDLER_H_

#include "base.h"

#include "core/device/sls/memblockhandler/block_ops.h"

namespace pnm::sls::device {

class CxlHardwareMemBlockHandler
    : public CxlMemBlockHandler<CxlHardwareMemBlockHandler> {
public:
  auto psum_reader() const { return RawBlockReader<true>{}; };
  auto tags_reader() const { return UnsupportedBlockReader{}; };
};
} // namespace pnm::sls::device

#endif // _SLS_CXL_HW_MEMBLOCKHANDLER_H_
