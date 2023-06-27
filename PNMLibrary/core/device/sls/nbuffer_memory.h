// core/device/sls/nbuffer_memory.h - sls n-buffered mem support *C++*//

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

#ifndef _SLS_NBUFFER_MEMORY_H_
#define _SLS_NBUFFER_MEMORY_H_

#include "memory_map.h"
#include "rank_address.h"

#include <linux/sls_resources.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace pnm::sls::device {

//!\brief Class for n-buffered memory support
class NBufferedMemory {
public:
  enum Type : uint8_t { READ, WRITE, MAX };

  NBufferedMemory(MemMap &memory, size_t single_buffer_size, size_t buffers_num,
                  sls_mem_blocks_e mem_block_type);

  pnm::sls::device::InterleavedPointer get_buffer(uint8_t rank,
                                                  Type type) const;
  pnm::sls::device::InterleavedPointer get_buffer_by_index(uint8_t rank,
                                                           uint8_t index) const;
  //!\brief Iterates over psum buf indices in range [0:NUM_OF_PSUM_BUF).
  void shift_buffers(uint8_t rank);
  uint8_t get_index(uint8_t rank, Type type) const;

private:
  void set_index(uint8_t rank, Type type, uint8_t value);
  void shift_index(uint8_t rank, Type type);
  void check_index_value(uint8_t value) const;

  static void check_index_access(uint8_t rank, Type type);

  MemMap &m_map_;
  const size_t single_buffer_size_;
  const size_t buffers_num_;
  std::vector<std::vector<uint8_t>> indices_;
  const sls_mem_blocks_e mem_block_type_;
};

} // namespace pnm::sls::device
#endif /* _SLS_NBUFFER_MEMORY_H_ */
