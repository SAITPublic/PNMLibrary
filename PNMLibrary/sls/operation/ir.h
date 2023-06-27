/*
 *  Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 *  This software is proprietary of Samsung Electronics.
 *  No part of this software, either material or conceptual may be copied or
 *  distributed, transmitted, transcribed, stored in a retrieval system or
 *  translated into any human or computer language in any form by any means,
 *  electronic, mechanical, manual or otherwise, or disclosed to third parties
 *  without the express written permission of Samsung Electronics.
 */

#ifndef PNM_SLS_IR_H
#define PNM_SLS_IR_H

#include <cstdint>

namespace pnm::sls {

/**@brief Intermediate representation of SLS instruction
 *
 * Here we store core information about SLS instruction that can be transformed
 * to target instruction in trace generator.
 *
 * Currently there are two targets:
 *  - Rankwise SLS
 *  - Channelwise SLS
 */
struct InstructionIR {
  uint32_t opcode;
  uint32_t output_index;
  uint64_t address;
};

inline InstructionIR encode_ir(uint32_t opcode, uint32_t output_index,
                               uint64_t table_addr) {
  return InstructionIR{opcode, output_index, table_addr};
}
} // namespace pnm::sls

#endif // PNM_SLS_IR_H
