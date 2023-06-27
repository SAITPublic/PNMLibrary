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

#ifndef SLS_ITABLES_GENERATOR_H
#define SLS_ITABLES_GENERATOR_H

#include "tools/datagen/sls/general/tables_info.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <ostream>
#include <stdexcept>
#include <vector>

/*! \brief Interface for tables generator. */
class ITablesGenerator {
public:
  auto create(const TablesInfo &info) { return create_table_impl(info); }

  void create_and_store(const std::filesystem::path &root,
                        const TablesInfo &info) {
    auto data = create(info);
    const auto tables_path = root / "embedded.bin";
    const auto info_path = root / "embedded.info";
    info.store_to_file(info_path);

    std::ofstream out(tables_path, std::ios::out | std::ios::binary);
    out.exceptions(std::ofstream::badbit | std::ostream::failbit);
    if (out) {
      out.write(reinterpret_cast<char *>(data.data()), data.size());
    } else {
      throw std::runtime_error(
          "Unable open embedded table file in write mode.");
    }
  }

  virtual ~ITablesGenerator() = default;

private:
  virtual std::vector<uint8_t> create_table_impl(const TablesInfo &info) = 0;
};
#endif // SLS_ITABLES_GENERATOR_H
