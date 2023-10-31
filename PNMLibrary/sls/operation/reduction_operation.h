/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or
 * computer language in any form
 * by any means, electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung
 * Electronics.
 */

#ifndef _REDUCTION_OPERATION_H_
#define _REDUCTION_OPERATION_H_

#include "core/operation/internal.h"

#include "pnmlib/common/views.h"

#include <cstdint>
#include <vector>

namespace pnm::sls {

/*
 @ brief: An abstract reduction operation to run on PNM hardware. The operation
 can be split to smaller parts (execs) due to hardware restrictions (e.g.
 INST/PSUM buffer sizes).
*/
class ReductionOperation : public InternalOperator {
public:
  /*
   @ brief: returns number of "micro" operations (execs) for this operation
            at pack.
  */
  virtual uint32_t get_num_exec_iterations(uint8_t pack) const = 0;

  /*
   @ brief: returns generated trace for exec_no'th exec at compute_unit.
  */
  virtual const std::vector<uint64_t> &
  generate_instructions(uint8_t pack, uint8_t compute_unit,
                        uint32_t exec_no) = 0;

  /*
   @ brief: get temporary output buffer
  */
  virtual pnm::views::common<uint8_t>
  get_tmp_buffer(uint8_t pack, uint32_t exec_no) const = 0;

  /*
   @ brief: write results to user output buffer
  */
  virtual void write_result(uint8_t output_no, uint8_t pack, uint32_t exec_no,
                            uint8_t *src) = 0;

  /*
   @ brief: get operation's outputs numbers
  */
  virtual uint8_t get_num_outputs() const = 0;

  ~ReductionOperation() override = default;
};

} // namespace pnm::sls

#endif
