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

#ifndef SLS_IBATCHES_GENERATOR_H
#define SLS_IBATCHES_GENERATOR_H

#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"

#include "pnmlib/common/misc_utils.h"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>

/*! \brief Interface for indices generators */
class IIndicesGenerator {
public:
  auto create(const IndicesInfo &indices_info, const TablesInfo &tables_info) {
    return create_table_impl(indices_info, tables_info);
  }

  void create_and_store(const std::filesystem::path &root,
                        const std::string &prefix,
                        const IndicesInfo &indices_info,
                        const TablesInfo &tables_info) {
    auto data = create_table_impl(indices_info, tables_info);
    const auto tables_path = root / (prefix + ".indices.bin");
    const auto info_path = root / (prefix + ".indices.info");
    indices_info.store_to_file(info_path);

    std::ofstream out(tables_path, std::ios::out | std::ios::binary);
    out.exceptions(std::ofstream::badbit);
    if (out) {
      out.write(reinterpret_cast<char *>(data.data()),
                pnm::utils::byte_size_of_container(data));
    } else {
      static const thread_local std::string error_message =
          std::string{
              "Unable open embedded table file in write mode. Reason: "} +
          strerror(errno);
      throw std::runtime_error(error_message.c_str());
    }
  }

  virtual ~IIndicesGenerator() = default;

private:
  virtual std::vector<uint32_t>
  create_table_impl(const IndicesInfo &indices_info,
                    const TablesInfo &tables_info) = 0;
};

#endif // SLS_IBATCHES_GENERATOR_H
