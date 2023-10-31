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

#include "common/log.h"
#include "common/profile.h"
#include "common/timer.h"

#include "pnmlib/sls/sls.h"

#include "pnmlib/common/compiler.h"

#include <mutex>

namespace {

std::mutex print_profiling_info_mutex;

void sls_report_profile_impl() {
  if (pnm::profile::get_profiling_flag()) {
    const std::lock_guard<std::mutex> guard(print_profiling_info_mutex);
    pnm::log::profile("PRINTING PROFILING INFO:");
    pnm::utils::NamedTimer::print_profiling_info();
  }
}

} // namespace

namespace pnm::profile {

PNM_API void sls_report_profile() { sls_report_profile_impl(); }

PNM_API void sls_start_profiling() { start_profiling(); }

PNM_API void sls_end_profiling() { end_profiling(); }

} // namespace pnm::profile
