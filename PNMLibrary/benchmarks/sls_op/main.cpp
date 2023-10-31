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

#include "dummy_cpu.h"

#include "secure/common/sls_executor.h"
#include "secure/common/sls_runner.h"
#include "secure/plain/device/trivial_cpu.h"
#include "secure/plain/run_strategy/producer_consumer.h"

#include "common/compiler_internal.h"
#include "common/mapped_file.h"
#include "common/profile.h"
#include "common/threads/workers_storage.h"

#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"
#include "tools/datagen/sls/indices_generator/base.h"
#include "tools/datagen/sls/indices_generator/factory.h"
#include "tools/datagen/sls/utils.h"

#include "pnmlib/secure/base_device.h"

#include "pnmlib/common/views.h"

#include "CLI/App.hpp"
#include "CLI/CLI.hpp" // NOLINT(misc-include-cleaner)
#include "CLI/Validators.hpp"

#include <fmt/core.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

struct Request {
  tools::gen::sls::IndicesInfo info{};
  size_t sparse_feature_size{};
  std::vector<uint32_t> array{};
};

std::queue<Request> request_queue;

class RequestGen {
public:
  explicit RequestGen(const std::filesystem::path &emb_path) {
    auto emb = tools::gen::sls::get_test_tables_mmap(emb_path);
    info_ = std::move(emb.info);
    igen_ = tools::gen::sls::IndicesGeneratorFactory::default_factory().create(
        "random");
  }

  void run(size_t requests_count) {
    while (requests_count > 0) {
      const std::vector<size_t> num_lookup(info_.rows().size(), num_lookup_);

      tools::gen::sls::IndicesInfo indices(num_lookup, minibatch_size_);

      auto array = igen_->create(indices, info_);

      const Request rq{std::move(indices), info_.cols(), std::move(array)};
      request_queue.push(rq);

      --requests_count;
    }

    fmt::print("All request created.\n");
  }

private:
  tools::gen::sls::TablesInfo info_;
  std::unique_ptr<tools::gen::sls::IIndicesGenerator> igen_;
  static constexpr auto minibatch_size_ = 256;
  static constexpr auto num_lookup_ = 40;
};

class Worker {
public:
  explicit Worker(const std::filesystem::path &emb_path) {
    static constexpr auto with_tag = false;
    auto emb = tools::gen::sls::get_test_tables_mmap(emb_path);
    num_tables_ = emb.info.num_tables();
    const pnm::sls::secure::DeviceArguments args{
        pnm::sls::secure::TrivialCpuArgs{
            .rows = pnm::views::make_view(emb.info.rows()),
            .sparse_feature_size = emb.info.cols(),
            .with_tag = with_tag}};
    runner_.init(&args);

    runner_.load_tables(emb.mapped_file.data(),
                        pnm::views::make_view(emb.info.rows()), emb.info.cols(),
                        with_tag);
  } // NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks) DeviceArguments handle
    // live time of his argument

  void run() {
    std::vector<uint8_t> psum;
    std::vector<uint8_t> checks;

    size_t processed = 0;
    fmt::print("Start listening request queue...\n");

    pnm::profile::TimerUsT timer_all;

    double wtime_100 = 0;

    timer_all.tick();
    while (!request_queue.empty()) {
      auto req = std::move(request_queue.front());
      request_queue.pop();

      psum.resize(req.sparse_feature_size * req.info.minibatch_size() *
                  num_tables_ * sizeof(uint32_t));
      checks.resize(req.info.minibatch_size() * num_tables_, 0);

      pnm::profile::TimerUsT run_timer;
      run_timer.tick();
      runner_.run(req.info.minibatch_size(),
                  pnm::views::make_view(req.info.lengths()),
                  pnm::views::make_view(std::cref(req.array).get()),
                  pnm::views::make_view(psum), pnm::views::make_view(checks));
      run_timer.tock();
      wtime_100 += run_timer.duration().count();

      ++processed;
      if (UNLIKELY(processed % 100 == 0)) {
        const auto average_wtime = (wtime_100 / 100.0);
        const auto request_enc_size =
            req.sparse_feature_size * req.info.num_requests() *
            req.info.lengths().front() * sizeof(uint32_t);
        const auto bandwidth = request_enc_size / average_wtime;

        // Conversion coefficient from bytes per ms to GB per s
        static constexpr auto BpMS2GBpS = 1e+6 / (1024 * 1024 * 1024);
        fmt::print("Processed {} request. Average time per request: {} micros. "
                   "Bandwidth: {} GB/s\n",
                   processed, average_wtime, bandwidth * BpMS2GBpS);
        wtime_100 = 0.0;
      }
    }
    timer_all.tock();
    fmt::print("End of all runs. Working time {} micros\n",
               timer_all.duration().count());
  }

private:
  pnm::sls::secure::Runner<pnm::sls::secure::OperationExecutor<
      uint32_t, DummyCPU<uint32_t>,
      pnm::sls::secure::SlsProducerConsumer<pnm::threads::Manager>>>
      runner_;
  size_t num_tables_;
};

int main(int argc, char **argv) {
  CLI::App app{"Benchmark for CPU SLS operation"};

  std::filesystem::path tables_root;

  app.add_option("tables_root", tables_root,
                 "Path to folder containing embedding tables data")
      ->required()
      ->check(CLI::ExistingDirectory);

  CLI11_PARSE(app, argc, argv);

  static constexpr auto request_count = 1000;

  pnm::profile::TimerUsT t;
  t.tick();
  Worker w(tables_root);
  t.tock();
  fmt::print("Data loading complete. Working time {} micros\n",
             t.duration().count());

  RequestGen rg(tables_root);
  rg.run(request_count);

  w.run();

  return 0;
}
