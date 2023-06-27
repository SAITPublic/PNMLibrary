/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#ifndef _PNM_CONTEXT_H_
#define _PNM_CONTEXT_H_

#include "allocator.h"
#include "device.h"
#include "dispatcher.h"
#include "transfer_manager.h"

#include "pnmlib/common/compiler.h"

#include <memory>

namespace pnm {

class InternalRunner;

class PNM_API Context {
public:
  auto type() const { return type_; }

  auto &allocator() { return allocator_; }

  auto &transfer_manager() { return transfer_manager_; }

  auto &device() { return device_; }

  auto &runner_core() { return runner_core_; }

  auto &dispatcher() { return operation_dispatcher_; }

  virtual ~Context();

protected:
  Context(Device::Type type, DevicePointer device,
          std::unique_ptr<memory::TransferManager> transfer_manager);

  void init_allocator(std::unique_ptr<memory::Allocator> allocator);

  void init_runner_core(std::unique_ptr<InternalRunner> runner);

private:
  Device::Type type_;

  DevicePointer device_;
  std::unique_ptr<memory::Allocator> allocator_;
  std::unique_ptr<memory::TransferManager> transfer_manager_;
  std::unique_ptr<InternalRunner> runner_core_;

  Dispatcher operation_dispatcher_;
};

using ContextHandler = std::unique_ptr<Context>;

ContextHandler make_context(Device::Type type);

} // namespace pnm

#endif //_PNM_CONTEXT_H_
