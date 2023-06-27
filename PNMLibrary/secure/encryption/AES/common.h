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

#ifndef SLS_AES_COMMON_H
#define SLS_AES_COMMON_H

namespace sls::secure {
enum class AES_KEY_SIZE : unsigned {
  AES_128 = 128U,
  AES_192 = 192U,
  AES_256 = 256U,
};
} // namespace sls::secure
#endif // SLS_AES_COMMON_H
