/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#ifndef _PNM_BUFFER_H_
#define _PNM_BUFFER_H_

#include "accessor.h"
#include "accessor_builder.h"
#include "context.h"

#include "pnmlib/core/memory.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/properties.h"
#include "pnmlib/common/views.h"

#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>

namespace pnm::memory {

template <typename T> class Buffer {
public:
  Buffer() = default;

  Buffer(uint64_t size, const ContextHandler &context,
         pnm::property::PropertiesList props = {})
      : context_{context.get()}, props_{std::move(props)}, size_{size},
        dev_region_{context_->allocator()->allocate(size_ * sizeof(T), props_),
                    true, getpid()} {
    if (!context_) {
      throw pnm::error::InvalidArguments(
          "Unable to create accessor to contextless buffer.");
    }
  }

  Buffer(pnm::common_view<T> user_range, const ContextHandler &context,
         pnm::property::PropertiesList props = {})
      : context_{context.get()}, user_range_{user_range},
        props_{std::move(props)},
        size_{static_cast<uint64_t>(user_range_.size())},
        dev_region_{context_->allocator()->allocate(size_ * sizeof(T), props_),
                    true, getpid()} {
    if (!context_) {
      throw pnm::error::InvalidArguments(
          "Unable to create accessor to contextless buffer.");
    }
    copy_to_device();
  }

  Buffer(pnm::common_view<const T> user_range, const ContextHandler &context,
         pnm::property::PropertiesList props = {})
      : context_{context.get()}, props_{std::move(props)},
        size_(static_cast<uint64_t>(user_range.size())),
        dev_region_{context_->allocator()->allocate(size_ * sizeof(T), props_),
                    true, getpid()} {
    if (!context_) {
      throw pnm::error::InvalidArguments(
          "Unable to create accessor to contextless buffer.");
    }

    context_->transfer_manager()->copy_to_device(
        pnm::make_view(const_cast<T *>(user_range.begin()),
                       const_cast<T *>(user_range.end())),
        direct_access());
  }

  Buffer(const DeviceRegion &dev_region, const ContextHandler &context,
         pnm::property::PropertiesList props = {})
      : context_{context.get()}, props_{std::move(props)},
        size_{std::visit(DeviceRegionSizeVisitor{}, dev_region) / sizeof(T)},
        dev_region_{dev_region, false, -1} {}

  Buffer(const Buffer &other) = delete;

  Buffer(Buffer &&other) noexcept
      : context_{std::exchange(other.context_, nullptr)},
        user_range_{std::exchange(other.user_range_, {})},
        offloaded_{other.offloaded_}, props_{std::exchange(other.props_, {})},
        size_{other.size_}, dev_region_{std::exchange(other.dev_region_, {})} {}

  Accessor<T> direct_access() {
    return Accessor<T>{create_accessor_core(dev_region_.value, context_)};
  }

  void bind_user_region(pnm::common_view<T> user_range) {
    if (static_cast<uint64_t>(user_range.size()) != size_) {
      throw pnm::error::InvalidArguments(
          "Unable to bind user region with size different from buffer size.");
    }

    user_range_ = user_range;
    offloaded_ = false;
  }

  uint64_t copy_to_device() {
    if (user_range_.empty()) {
      throw pnm::error::InvalidArguments(
          "Unable to offload empty user region.");
    }

    auto count = context_->transfer_manager()->copy_to_device(user_range_,
                                                              direct_access());

    offloaded_ = true;
    return count;
  }

  uint64_t copy_from_device() {
    if (user_range_.empty()) {
      throw pnm::error::InvalidArguments(
          "Unable to upload into empty user region.");
    }

    return context_->transfer_manager()->copy_from_device(direct_access(),
                                                          user_range_);
  }

  auto size() const { return size_; }

  const auto &context() const { return context_; }

  const auto &device_region() const { return dev_region_.value; }

  auto is_offloaded() const { return offloaded_; }

  auto is_device_region_owned() const { return dev_region_.owned; }

  auto &operator=(const Buffer &other) = delete;

  Buffer &operator=(Buffer &&other) noexcept {
    release();
    context_ = std::exchange(other.context_, nullptr);
    user_range_ = std::exchange(other.user_range_, {});
    offloaded_ = other.offloaded_;
    props_ = std::exchange(other.props_, {});
    size_ = other.size_;
    dev_region_ = std::exchange(other.dev_region_, {});
    return *this;
  }

  ~Buffer() {
    if (dev_region_.owned && dev_region_.owner_process_id == getpid()) {
      release();
    }
  }

private:
  void release() noexcept {
    if (!context_) {
      return;
    }

    try {
      context_->allocator()->deallocate(dev_region_.value);
    } catch (pnm::error::Base &e) {
      //[TODO: y-lavrinenko] Use next code when PNM_LOG_ERROR be public
      //        PNM_LOG_ERROR("Fail to deallocate {} bytes in buffer {}. Reason:
      //        {}", size_,
      //                  static_cast<void *>(this), e.what());
    }
  }

  struct DeviceRegionWrapper {
    DeviceRegion value;
    bool owned;

    // This field solves double free error when buffers are used in forked
    // processes. After fork we have 2 or more buffers that refer to the same
    // memory region. As a result, in dtor each buffer in each process makes
    // DEALLOCATE request multiple times for the same memory region that leads
    // to a crash in genalloc. So, here we store the owner process id into the
    // owner_process_id field. In dtor we compare the current process id with
    // it. If values are equal we make a deallocation, otherwise we just skip.
    // [TODO: MCS23-1212] Remove this field after process manager improvement
    pid_t owner_process_id;
  };

  struct DeviceRegionSizeVisitor {
    uint64_t operator()(const SequentialRegion &reg) { return reg.size; }

    uint64_t operator()(const RankedRegion &reg) {
      return reg.regions.empty() ? 0 : this->operator()(reg.regions[0]);
    }
  };

  Context *context_{nullptr};

  pnm::common_view<std::decay_t<T>> user_range_{};
  bool offloaded_{false};

  pnm::property::PropertiesList props_;
  uint64_t size_; // The number of elements of type T in buffer
  DeviceRegionWrapper dev_region_;
};
} // namespace pnm::memory

#endif //_PNM_BUFFER_H_
