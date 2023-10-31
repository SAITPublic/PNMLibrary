/*
 * Copyright (C) 2021 Samsung Electronics Co. LTD
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

/** @file sls.h
    @brief API functions for SLS device library.
*/

#ifndef _SLS_H_
#define _SLS_H_

namespace pnm::profile {

/***** SLS APIs for customer *****/

void sls_report_profile();

void sls_start_profiling();

void sls_end_profiling();

} // namespace pnm::profile

#endif
