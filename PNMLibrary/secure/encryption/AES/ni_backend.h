/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 */

#ifndef SLS_AES_NI_BACKEND_H
#define SLS_AES_NI_BACKEND_H

#include "common.h"

#include <cstdint>
#include <memory>

namespace sls::secure {
/*! \brief The encryption strategy that use Intel AES-NI instruction.
 * This class can be used as a backend for AES_Engine.
 */
class AES_NI_Backend {
public:
  AES_NI_Backend(AES_KEY_SIZE key_size, const uint8_t *key);

  int encrypt(const uint8_t *data, int length, uint8_t *out) const;

private:
  unsigned const encryption_rounds_;
  std::unique_ptr<uint8_t[]> key_schedule_;
};
} // namespace sls::secure

#endif // SLS_AES_NI_BACKEND_H
