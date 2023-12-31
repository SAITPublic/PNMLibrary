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

#ifndef _SLS_CONTEXT_MOCK_UTILS_H
#define _SLS_CONTEXT_MOCK_UTILS_H

#include "pnmlib/core/context.h"

#include <cstdint>
#include <memory>
#include <string>

namespace test::mock {

enum class ContextType : uint8_t {
  SLS,
  RANDOM_OFFSETS_ALLOCATOR,
};

std::unique_ptr<pnm::Context> make_context_mock(ContextType);
std::string format_as(ContextType type);

} // namespace test::mock
#endif
