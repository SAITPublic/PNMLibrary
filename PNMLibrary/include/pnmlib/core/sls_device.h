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

/** @file sls_device.h
 *  @brief Contains class for accessing the sls device
 */

#ifndef _SLS_DEVICE_H_
#define _SLS_DEVICE_H_

#include "pnmlib/core/device.h"
#include "pnmlib/core/sls_cunit_info.h"

#include "pnmlib/common/compiler.h"

#include <linux/sls_resources.h> // NOLINT(misc-include-cleaner)

#include <cstddef>
#include <cstdint>
#include <memory>

/***** SLS APIs for customer *****/
namespace pnm::sls::device {
class BaseDevice;
} // namespace pnm::sls::device

// [TODO: @e-kutovoi] Essentially this is just a wrapper for BaseDevice,
// because right now BaseDevice is way to big. We can't make the user
// include `core/device/sls/base.h` because it includes many internal
// headers, which in turn depend on `fmt` and `spdlog`. These libraries are not
// our public dependencies, so we can't include them in public headers.
//
// After we clean up BaseDevice and finalize what exactly will be our
// API, we should get rid of this class and use BaseDevice or similar

/** @brief Context-like object that provides a frontend for getting low-level
 * device information
 */
class PNM_API SlsDevice {
public:
  SlsDevice() noexcept;
  SlsDevice(SlsDevice &&) noexcept;
  SlsDevice &operator=(SlsDevice &&) noexcept;
  ~SlsDevice();

  /** @brief Initialize (open) the device
   *
   * @throws pnm::error::InvalidArguments if already opened, pnm::error::IO if
   * device driver not loaded
   */
  void open(pnm::Device::Type dev_type);

  /** @brief check if device is initialized
   */
  bool is_open() const noexcept;

  /** @brief Close the device
   *
   * @throws pnm::error::InvalidArguments if already closed, pnm::error::IO if
   * device is corrupted
   */
  void close();

  /** @brief Create and fully initialize the device. Use this to create devices
   * in cases when you don't need to delay initialization
   *
   * @throws pnm::error::IO if device driver not loaded
   */
  static SlsDevice make(pnm::Device::Type dev_type);

  /** @brief Get device SLS device memory size.
   */
  size_t base_memory_size() const noexcept;

  /** @brief Get various information about specified compute unit
   *
   * @param[in] compute_unit SLS device compute unit number [0 : NUM_OF_RANK -
   * 1]
   * @param[in] key Available keys can be found here
   * ::sls::device::Control::compute unit_info_paths.
   *
   * @throws pnm::error::IO
   *
   * @return Value of requested field
   */
  uint64_t compute_unit_info(uint8_t compute_unit,
                             SlsComputeUnitInfo key) const;

  /** @brief Reset device via sysfs. Requires root capabilities
   *
   * @throws pnm::error::IO
   */
  void reset();

  /** @brief Get timeout for compute unit acquisition
   *
   * @throws pnm::error::IO
   */
  uint64_t acquisition_timeout() const;

  /** @brief Set timeout for compute unit acquisition. Requires root
   * capabilities.
   *
   * @throws pnm::error::IO
   */
  void set_acquisition_timeout(uint64_t value);

  /** @brief Get number of processes which didn't free their resources: compute
   * unit acquisitions, memory allocations.
   *
   * @throws pnm::error::IO
   */
  uint64_t leaked_count() const;

  /** @brief Get status of leaked resources cleanup feature
   *
   * @throws pnm::error::IO
   */
  bool resource_cleanup() const;

  /** @brief Enable/disable leaked resources cleanup feature. Root capabilities
   * required
   *
   * @throws pnm::error::IO
   */
  void set_resource_cleanup(bool value);

  /** @brief Should only be used by internal code! But even then, try to refrain
   * from it and consider adding methods to public API if existing API doesn't
   * suffice
   *
   * @return Ptr to the underlying device object
   */
  pnm::sls::device::BaseDevice *internal() noexcept;
  const pnm::sls::device::BaseDevice *internal() const noexcept;

private:
  std::unique_ptr<pnm::sls::device::BaseDevice> device_;
};

#endif //_SLS_DEVICE_H_
