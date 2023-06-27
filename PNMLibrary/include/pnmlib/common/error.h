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

#ifndef PNM_ERROR_H
#define PNM_ERROR_H

#include "pnmlib/common/compiler.h"

#include <exception>
#include <string>
#include <utility>

namespace pnm::error {

/** @brief Base class for all our user-facing exceptions. Don't throw this
 * directly, but catch if you don't care about the exact type. When throwing
 * exceptions, pick any one below
 */
class PNM_API Base : public std::exception {
public:
  explicit Base(std::string message) : message_(std::move(message)) {}

  const char *what() const noexcept override { return message_.c_str(); }

private:
  std::string message_;
};

/** @brief Throw when too much memory was requested
 */
class PNM_API OutOfMemory : public Base {
public:
  explicit OutOfMemory(const std::string &message)
      : Base("OutOfMemory: " + message) {}
};

/** @brief Throw when direct memory accesses fail
 */
class PNM_API BadAccess : public Base {
public:
  explicit BadAccess(const std::string &message)
      : Base("BadAccess: " + message) {}
};

/** @brief Throw when user supplied incorrect arguments to APIs
 */
class PNM_API InvalidArguments : public Base {
public:
  explicit InvalidArguments(const std::string &message)
      : Base("InvalidArguments: " + message) {}
};

/** @brief Throw when any errors related to device communications are found,
 * e.g. when accessing sysfs/device driver/mmaping
 */
class PNM_API IO : public Base {
public:
  explicit IO(const std::string &message) : Base("IO: " + message) {}
};

/** @brief Throw when the requested action is not allowed
 */
class PNM_API NotSupported : public Base {
public:
  explicit NotSupported(const std::string &message)
      : Base("NotSupported: " + message) {}
};

/** @brief Generic failure, use when nothing more specific applies
 */
class PNM_API Failure : public Base {
public:
  explicit Failure(const std::string &message) : Base("Failure: " + message) {}
};

} // namespace pnm::error

#endif // PNM_ERROR_H
