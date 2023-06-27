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

#ifndef IMDB_SCAN_IMDB_H
#define IMDB_SCAN_IMDB_H

#include "core/operation/internal.h"

#include "pnmlib/imdb/libimdb.h"
#include "pnmlib/imdb/scan.h"

#include <cstdint>

namespace pnm::imdb {

/** @brief This takes in multiple input and output buffers and encodes this
 * information to be forwarded to the device itself
 */
class ScanOperation : public pnm::InternalOperator {
public:
  ScanOperation(pnm::operations::Scan &op) : frontend_{op} {}

  auto result_size() const { return result_size_; }
  auto execution_time() const { return execution_time_; }

  ThreadCSR encode_to_registers() const {
    ThreadCSR registers{};
    write_registers(&registers);
    return registers;
  }

  void write_registers(volatile ThreadCSR *ptr) const;

  void read_registers(const volatile ThreadCSR *ptr);

private:
  pnm::operations::Scan &frontend_;

  void set_predicate_info(volatile ThreadCSR *ptr) const;
  void set_output_type(volatile ThreadCSR *ptr) const;

  uint64_t result_size_ = 0;
  uint64_t execution_time_ = 0;
};

} // namespace pnm::imdb

#endif // IMDB_SCAN_IMDB_H
