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

#ifndef SLS_GENERATOR_FACTORY_H
#define SLS_GENERATOR_FACTORY_H

#include "base.h"

#include "tools/datagen/common_factory.h"

#include <memory>
#include <string>

/*! \brief Fabric for table generators. Call default_factory() method to get
 * fabric with all preregistered generators.*/
class TablesGeneratorFactory
    : public CommonFactory<std::string, std::unique_ptr<ITablesGenerator>,
                           const std::string &> {
public:
  static TablesGeneratorFactory &default_factory();
};

#endif // SLS_GENERATOR_FACTORY_H
