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

#ifndef PNM_ERROR_MESSAGE_H
#define PNM_ERROR_MESSAGE_H

#include <string_view>

namespace pnm::error {

inline constexpr std::string_view open_file =
    "Unable to open file. Path: {}. Reason: {}.";

inline constexpr std::string_view open_common =
    "Unable to open {}. Reason: {}.";
inline constexpr std::string_view close_common =
    "Unable to close {}. Reason: {}.";

inline constexpr std::string_view mmap_file =
    "Unable to mmap file. Path: {}. File descriptor: {}. Length: {}. Offset: "
    "{}. Reason: {}.";
inline constexpr std::string_view mmap_device =
    "Unable to mmap device. File descriptor: {}. Size: {}. (Aligned: {}). "
    "Offset: {}.  Reason: {}.";
inline constexpr std::string_view mmap_common =
    "Unable to mmap {}. File descriptor: {}. Length: {}. Offset: {}. Reason: "
    "{}.";

inline constexpr std::string_view munmap_memory =
    "Unable to munmap memory. Address: {:x}. Length: {}. Reason: {}.";

inline constexpr std::string_view write_to_file =
    "Unable to write value to file. Path: {}.";
inline constexpr std::string_view read_from_file =
    "Unable to read value from file. Path: {}.";

inline constexpr std::string_view from_errno = "Error: \"{}\"";
inline constexpr std::string_view ioctl_from_errno = "Ioctl error: \"{}\"";

} // namespace pnm::error

#endif // PNM_ERROR_MESSAGE_H
