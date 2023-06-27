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

#ifndef SLS_TEST_TABLES_MMAP_H
#define SLS_TEST_TABLES_MMAP_H

#include "tables_info.h"

#include "common/mapped_file.h"

#include <filesystem>

namespace sls::tests {

/*! \brief Class that allows to read embedded tables via mmap */
class TablesMmap {
public:
  TablesMmap() = default;

  TablesMmap(const std::filesystem::path &data_path,
             const std::filesystem::path &info_path)
      : mapped_file(data_path), info(info_path) {}

  pnm::utils::MappedFile mapped_file;
  TablesInfo info;
};

} // namespace sls::tests

#endif // SLS_TEST_TABLES_MMAP_H
