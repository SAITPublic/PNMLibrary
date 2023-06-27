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

#ifndef _PRODUCER_CONSUMER_RUNNER_H_
#define _PRODUCER_CONSUMER_RUNNER_H_

#include "common/threads/producer_consumer.h"

namespace sls::secure {
template <typename ThreadCreationStrategy>
using SLSProducerConsumer =
    pnm::threads::ProducerConsumer<ThreadCreationStrategy>;
} // namespace sls::secure

#endif //_PRODUCER_CONSUMER_RUNNER_H_
