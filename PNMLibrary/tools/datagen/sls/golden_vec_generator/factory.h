
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

#ifndef SLS_SLS_PROCESSOR_H
#define SLS_SLS_PROCESSOR_H

#include "tools/datagen/common_factory.h"
#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/general/test_tables_mmap.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace tools::gen::sls {

/*! \brief Interface for SLS operation */
class IGoldenVecGenerator {
public:
  virtual ~IGoldenVecGenerator() = default;

  auto compute_golden_sls(const void *tables, const TablesInfo &tables_meta,
                          const uint32_t *indices,
                          const IndicesInfo &indices_meta) const {
    return compute_golden_sls_impl(tables, tables_meta, indices, indices_meta);
  }

  void compute_and_store_golden_sls(const std::filesystem::path &root,
                                    const std::string &prefix,
                                    TablesMmap &tables, Indices &indices) const;

private:
  virtual std::vector<uint8_t>
  compute_golden_sls_impl(const void *tables, const TablesInfo &tables_meta,
                          const uint32_t *indices,
                          const IndicesInfo &indices_meta) const = 0;
};

/*! \brief Factory that build sls processors */
class GoldenVecGeneratorFactory
    : public CommonFactory<std::string, std::unique_ptr<IGoldenVecGenerator>> {
public:
  static GoldenVecGeneratorFactory &default_factory();
};

} // namespace tools::gen::sls

#endif // SLS_SLS_PROCESSOR_H
