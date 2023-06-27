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

#ifndef SLS_AES_KEY_EXP192_H
#define SLS_AES_KEY_EXP192_H

#include "common/make_error.h"

#include <emmintrin.h>
#include <wmmintrin.h>

inline void KEY_192_ASSIST([[maybe_unused]] __m128i *temp1,
                           [[maybe_unused]] __m128i *temp2,
                           [[maybe_unused]] __m128i *temp3) {
  throw pnm::error::make_not_sup("AES-NI is not supported!");
}

inline void AES_192_Key_Expansion([[maybe_unused]] const unsigned char *userkey,
                                  [[maybe_unused]] unsigned char *key) {
  throw pnm::error::make_not_sup("AES-NI is not supported!");
}

#endif // SLS_AES_KEY_EXP192_H
