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

#ifndef _DUMMY_CPU_H
#define _DUMMY_CPU_H

#include "secure/common/sls_io.h"

#include "pnmlib/secure/base_device.h"

#include "pnmlib/common/views.h"

#include <cstdint>

/*! \brief Class that perform SLS on CPU.
 *
 * This class is one of a set of runners that called from OperationExecutor.
 * Class has run method with same argument as a AxDIMM API.
 *
 * @tparam T is a type of table's entries.
 * */
template <typename T> class DummyCPU : public pnm::sls::secure::IDevice {
public:
  using MemoryReader = pnm::sls::secure::TrivialMemoryReader;
  using MemoryWriter = pnm::sls::secure::TrivialMemoryWriter;

private:
  void init_impl(
      [[maybe_unused]] const pnm::sls::secure::DeviceArguments *args) override {
  }

  void load_impl([[maybe_unused]] const void *src,
                 [[maybe_unused]] uint64_t size_bytes) override {}

  void run_impl([[maybe_unused]] uint64_t minibatch_size,
                [[maybe_unused]] pnm::views::common<const uint32_t> lengths,
                [[maybe_unused]] pnm::views::common<const uint32_t> indices,
                [[maybe_unused]] pnm::views::common<uint8_t> psum) override {}
};

#endif // _DUMMY_CPU_H
