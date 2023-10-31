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

#include "utils.h"

#include "context_mock.h"

#include "common/make_error.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"

#include <memory>
#include <string>

namespace test::mock {

std::unique_ptr<pnm::Context> make_context_mock(ContextType type) {
  switch (type) {
  case ContextType::SLS:
    return pnm::make_context(pnm::Device::Type::SLS);
  case ContextType::RANDOM_OFFSETS_ALLOCATOR:
    return create_mock_context();
  }
  throw pnm::error::make_inval("Unknown context mock type: {}.",
                               static_cast<int>(type));
}

std::string format_as(ContextType type) {
  switch (type) {
  case ContextType::SLS:
    return "SLS";
  case ContextType::RANDOM_OFFSETS_ALLOCATOR:
    return "RANDOM_OFFSETS_ALLOCATOR";
  }
  throw pnm::error::make_inval("Unknown context type: {}.",
                               static_cast<int>(type));
}

} // namespace test::mock
