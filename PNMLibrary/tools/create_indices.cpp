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

#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/golden_vec_generator/factory.h"
#include "tools/datagen/sls/indices_generator/factory.h"
#include "tools/datagen/sls/lengths_generator/factory.h"
#include "tools/datagen/sls/utils.h"

#include "CLI/App.hpp"
#include "CLI/CLI.hpp" // NOLINT(misc-include-cleaner)
#include "CLI/Error.hpp"
#include "CLI/Validators.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

int main(int argc, char **argv) {
  CLI::App app{
      "Utility to create synthetic indices data for testing SLS operation"};

  std::filesystem::path tables_root;
  std::string prefix;
  std::optional<size_t> min_num_lookup{};
  size_t max_num_lookup{};
  size_t minibatch_size{};
  std::string lengths_generator_name;
  std::string overflow_type;
  std::string generator_name;
  std::string generator_args;
  std::string entry_type;

  app.add_option("path", tables_root,
                 "Path to the root directory where test tables will be located")
      ->check(CLI::ExistingDirectory)
      ->required();
  app.add_option("prefix", prefix, "Prefix of indices file")
      ->default_val("idx");
  app.add_option("--ml,--min_num_lookup", min_num_lookup,
                 "Min number of lookups for each table (the number of indices "
                 "in single batch)")
      ->default_str("max_num_lookup");
  app.add_option("--Ml,--max_num_lookup", max_num_lookup,
                 "Max number of lookups for each table (the number of indices "
                 "in single batch)");
  app.add_option("--bs,--minibatch_size", minibatch_size,
                 "Number of batches per table")
      ->default_val(1);
  app.add_option(
         "--lg,--lengths_gen", lengths_generator_name,
         "Name of generator that creates lengths.\n"
         "    random = totally random in range [min_num_lookup, "
         "max_num_lookup]\n"
         "    random_lookups_per_table = generate num_tables random values "
         "(lookups per table) and duplicate each minibatch_sizes times")
      ->check(CLI::IsMember({"random", "random_from_lookups_per_table"}))
      ->default_val("random_from_lookups_per_table");
  app.add_option("--ot,--overflow_type", overflow_type,
                 "Enable psum/inst buffer overflow in each N-th table in "
                 "random lengths generator.\n"
                 "Possible values:\n"
                 "    0 = without overflow, num_tables * minibatch_size "
                 "generated random values\n"
                 "    n = same as 0, but with instruction buffer overflow in "
                 "every n-th table\n"
                 "    -1 = same as 0, but with instruction buffer overflow in "
                 "one random table")
      ->default_val("0");
  app.add_option("-g,--generator", generator_name,
                 "Name of generator that creates indices.\n"
                 "    random = random values\n"
                 "    sequential = values in range [0, minibatch_size)")
      ->check(CLI::IsMember({"random", "sequential"}))
      ->default_val("random");
  app.add_option("--ga,--gargs", generator_args, "Arguments for generator");
  app.add_option("--et,--entry_t", entry_type,
                 "Type of tables' entries. Required for golden psum vec")
      ->check(CLI::IsMember({"uint32_t", "uint64_t", "float"}))
      ->default_val("uint32_t");

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    return app.exit(e);
  }

  auto indices_generator = IndicesGeneratorFactory::default_factory().create(
      generator_name, generator_args);

  auto length_generator = LengthsGeneratorFactory::default_factory().create(
      lengths_generator_name, overflow_type);

  auto tables = sls::tests::get_test_tables_mmap(tables_root);
  const TablesInfo &tinfo = tables.info;

  auto lengths = length_generator->create(tinfo.num_tables(), min_num_lookup,
                                          max_num_lookup, minibatch_size);
  const IndicesInfo info(minibatch_size, lengths);
  indices_generator->create_and_store(tables_root, prefix, info, tinfo);

  auto indices = sls::tests::get_test_indices(tables_root, prefix);

  auto generator =
      GoldenVecGeneratorFactory::default_factory().create(entry_type);
  generator->compute_and_store_golden_sls(tables_root, prefix, tables, indices);

  return 0;
}
