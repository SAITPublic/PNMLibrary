
#ifndef _COMPUTE_SLS_DEVICE_H_
#define _COMPUTE_SLS_DEVICE_H_

#include "core/device/sls/base.h"
#include "core/device/sls/utils/constants.h"

#include "common/memory/atomic.h"

#include <cassert>
#include <cstdint>
#include <utility>

namespace pnm::sls::device {

/** @brief Core device for SLS compute units.
 * At the moment it simply redirects calls to BaseDevice.
 * Simulator devcices inherit from it and implement specific behaviour.
 */
class ComputeDevice {
public:
  ComputeDevice(BaseDevice *device) : device_(device) { assert(device_); }

  template <typename... Args> void read_and_reset_psum(Args &&...args) {
    device_->read_and_reset_psum(std::forward<Args>(args)...);
  }

  template <typename... Args> void read_and_reset_tags(Args &&...args) {
    device_->read_and_reset_tags(std::forward<Args>(args)...);
  }

  template <typename... Args> void write_inst_block(Args &&...args) {
    return device_->write_inst_block(std::forward<Args>(args)...);
  }

  void write_exec_sls_register(uint8_t compute_unit, uint32_t val) {
    memory::hw_atomic_store(device_->exec_sls_register(compute_unit), val);
  }

  void write_control_switch_register(uint8_t compute_unit, uint32_t val) {
    memory::hw_atomic_store(device_->control_switch_register(compute_unit),
                            val);
  }

  bool acquire_cunit_from_range(uint8_t *compute_unit, cunit cunit_mask) {
    return device_->acquire_cunit_from_range(compute_unit, cunit_mask);
  }

  void release_cunit(uint8_t compute_unit) const {
    device_->release_cunit(compute_unit);
  }

  auto psum_poll_register(uint8_t compute_unit) const {
    return device_->psum_poll_register(compute_unit);
  }

  auto control_switch_register(uint8_t compute_unit) {
    return device_->control_switch_register(compute_unit);
  }

  auto get_device_type() const { return device_->get_device_type(); }

protected:
  BaseDevice *device_;
};

} // namespace pnm::sls::device

#endif
