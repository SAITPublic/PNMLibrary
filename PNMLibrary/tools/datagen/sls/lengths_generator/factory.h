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

#ifndef SLS_LENGTHS_GEN_FACTORY_H
#define SLS_LENGTHS_GEN_FACTORY_H

#include "base.h"

#include "tools/datagen/common_factory.h"

#include <memory>
#include <string>

class LengthsGeneratorFactory
    : public CommonFactory<std::string, std::unique_ptr<ILengthsGenerator>,
                           const std::string &> {
public:
  static LengthsGeneratorFactory &default_factory();
};

#endif // SLS_LENGTHS_GEN_FACTORY_H
