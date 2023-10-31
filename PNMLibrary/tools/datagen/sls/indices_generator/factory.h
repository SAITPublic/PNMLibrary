
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

#ifndef SLS_INDICES_GEN_FACTORY_H
#define SLS_INDICES_GEN_FACTORY_H

#include "base.h"

#include "tools/datagen/common_factory.h"

#include <memory>
#include <string>

namespace tools::gen::sls {

/*!\brief Factory for Indices generators. Call default_factory() to get factory
 * with preregistered generators. */
class IndicesGeneratorFactory
    : public CommonFactory<std::string, std::unique_ptr<IIndicesGenerator>,
                           const std::string &> {
public:
  static IndicesGeneratorFactory &default_factory();
};

} // namespace tools::gen::sls

#endif // SLS_INDICES_GEN_FACTORY_H
