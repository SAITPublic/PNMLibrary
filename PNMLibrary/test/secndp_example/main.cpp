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

#include "secure/plain/sls.h"

#include "common/topology_constants.h"

#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/indices_generator/factory.h"
#include "tools/datagen/sls/tables_generator/factory.h"

#include "pnmlib/secure/base_device.h"
#include "pnmlib/secure/base_runner.h"
#include "pnmlib/secure/untrusted_sls_params.h"

#include "pnmlib/common/rowwise_view.h"
#include "pnmlib/common/views.h"

#include "CLI/App.hpp"
#include "CLI/CLI.hpp" // NOLINT(misc-include-cleaner)

#include <fmt/core.h>
#include <fmt/format.h>

#include <linux/sls_resources.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

using namespace tools::gen::sls;

int main(int argc, char **argv) {
  CLI::App app{"Example SecNDP API application"};

  bool with_tag{};

  app.add_flag("-t,--tag", with_tag, "Use MAC tags for verification")
      ->default_val(false);

  CLI11_PARSE(app, argc, argv);

  if (with_tag &&
      pnm::sls::device::topo().Bus == pnm::sls::device::BusType::CXL) {
    fmt::print(stderr, "SLS-CXL HW does not support tags\n");
    return 0;
  }

  // 1. Prepare input tables
  // For explanation about the internal structure see
  // `docs/howto/embedding_tables.md`

  // Each table has the same number of columns (i.e. the features that each
  // table represents are vectors of the same length)MakeBufferFromSharedMP
  static constexpr size_t column_count = 16;
  // Number of rows per table
  const std::vector<uint32_t> table_row_sizes = {6, 7, 5};

  const TablesInfo tinfo(table_row_sizes.size(), column_count, table_row_sizes);
  auto tgen = TablesGeneratorFactory::default_factory().create("position");
  auto emb_tables = tgen->create(tinfo);

  // 2. Prepare input indices
  // For explanation about the internal structure see
  // `docs/howto/generate_indices.md`

  // Number of indices in each SLS query per table
  const std::vector<size_t> num_of_indices_per_table = {4, 3, 4};
  // A minibatch consists of some number of corresponding SLS queries for
  // each table
  static constexpr size_t queries_per_table = 2;

  const IndicesInfo iinfo(num_of_indices_per_table, queries_per_table);
  auto igen = IndicesGeneratorFactory::default_factory().create("sequential");
  const auto indices = igen->create(iinfo, tinfo);

  // 3. Setup device parameters
  const pnm::sls::secure::UntrustedDeviceParams sls_device_params{
      // Numbers of rows per table
      .rows = pnm::views::make_view(tinfo.rows()),
      // The amount of columns
      .sparse_feature_size = tinfo.cols(),
      // To use encryption verification or not (this adds some additional bytes
      // to be passed with the encrypted data itself)
      .with_tag = with_tag,
      // The strategy determining how tables are arranged on the available ranks
      .preference = SLS_ALLOC_DISTRIBUTE_ALL};

  // 4. Put the parameters into a generic container
  auto device_arguments = pnm::sls::secure::DeviceArguments(sls_device_params);

  // 5. Create and initialize the runner
  std::unique_ptr<pnm::sls::secure::IRunner> runner =
      std::make_unique<pnm::sls::secure::ProdConsSlsRunner<uint32_t>>();

  runner->init(&device_arguments);

  // 6. Load the table data onto the device. This function firstly encrypts the
  //    data, and only then loads it onto the untrusted device
  runner->load_tables(emb_tables.data(), pnm::views::make_view(tinfo.rows()),
                      tinfo.cols(), with_tag);

  // 7. Allocate data for the results
  auto requests_count = tinfo.num_tables() * iinfo.minibatch_size();
  // Result of the SLS operations
  std::vector<uint32_t> psum(tinfo.cols() * requests_count);
  // Results of MAC verification per request
  std::vector<uint8_t> row_checks(requests_count, false);

  // 8. Perform the computations
  const bool verification_result = runner->run(
      iinfo.minibatch_size(), pnm::views::make_view(iinfo.lengths()),
      pnm::views::make_view(indices),
      pnm::views::view_cast<uint8_t>(pnm::views::make_view(psum)),
      pnm::views::make_view(row_checks));

  fmt::print("Verification result: {}\n",
             verification_result ? "CORRECT" : "INCORRECT");
  fmt::print("Psum results:\n");

  auto psum_view = pnm::views::make_rowwise_view(
      psum.data(), psum.data() + psum.size(), tinfo.cols());

  for (const auto &vector : psum_view) {
    fmt::print("{}\n", fmt::join(vector, "\t"));
  }

  return 0;
}
