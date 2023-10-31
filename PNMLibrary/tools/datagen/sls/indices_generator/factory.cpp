
/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#include "factory.h"

#include "random.h"
#include "sequential.h"

#include <memory>

namespace tools::gen::sls {

IndicesGeneratorFactory &IndicesGeneratorFactory::default_factory() {
  static IndicesGeneratorFactory factory;

  factory.register_builder("random", []([[maybe_unused]] const auto &args) {
    return std::make_unique<RandomIndicesGen>();
  });

  factory.register_builder("sequential", []([[maybe_unused]] const auto &args) {
    return std::make_unique<SequentialIndicesGen>();
  });
  return factory;
}

} // namespace tools::gen::sls
