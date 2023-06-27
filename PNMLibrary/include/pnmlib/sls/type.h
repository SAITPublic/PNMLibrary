/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
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

#ifndef _SLS_TYPE_H_
#define _SLS_TYPE_H_

#include <cstdint>

// NOTE: The values of the enum are used as an opcode for operations, so we fix
// them inplace

/** @brief SLS operation types. Specifies type of embeddings used in SLS
 * calculation.
 */
enum class SLSType : uint32_t {
  /** @brief Treat each sparse feature element(embedding vector coordinate) as
   * float
   */
  Float = 0U,
  /** @brief Treat each sparse feature element(embedding vector coordinate) as
   * uint32
   */
  Uint32 = 2U,
  /** @brief Treat each sparse feature element(embedding vector coordinate) as
   * uint32 and verify calculations result using tags.
   */
  Uint32Tagged = 3U
};

#endif // _SLS_TYPE_H_
