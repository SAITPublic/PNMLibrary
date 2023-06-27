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

#ifndef _THREAD_NAME_H_
#define _THREAD_NAME_H_

#include <features.h>

#include <algorithm>
#include <cstddef>
#include <string>

#ifndef __GLIBC_PREREQ
#define __GLIBC_PREREQ(x, y) 0
#endif

#if defined(__GLIBC__) && __GLIBC_PREREQ(2, 12)
#define HAVE_PTHREAD_SETNAME_NP
#endif

#ifdef HAVE_PTHREAD_SETNAME_NP
#include <pthread.h>
#endif

namespace pnm::threads {

inline void set_thread_name(std::string name) {
#ifdef HAVE_PTHREAD_SETNAME_NP
  // This is defined by Glibc according to 'man pthread_setname_np'.
  constexpr size_t kMaxThreadName = 15;
  name.resize(std::min(name.size(), kMaxThreadName));

  pthread_setname_np(pthread_self(), name.c_str());
#endif
}

} // namespace pnm::threads

#endif
