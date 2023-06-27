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

#ifndef SLS_COMMON_FACTORY_H
#define SLS_COMMON_FACTORY_H

#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

/*! \brief Common factory template */
template <typename KeyT, typename ReturnType, typename... BuildArgs>
class CommonFactory {
public:
  using build_function = std::function<ReturnType(BuildArgs &&...)>;

  void register_builder(KeyT gen_name, build_function builder) {
    builders_.emplace(std::move(gen_name), std::move(builder));
  }

  auto create(const KeyT &gen_name, BuildArgs &&...args) const {
    try {
      return builders_.at(gen_name)(std::forward<BuildArgs>(args)...);
    } catch (std::out_of_range &e) {
      auto to_string = [](const auto &v) {
        if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
          return v;
        } else {
          return std::to_string(v);
        }
      };

      static thread_local std::string emsg =
          "Unable to create generator with name " + to_string(gen_name) +
          ".Builder not found.";
      throw std::runtime_error(emsg.c_str());
    }
  }

  auto create(const std::string &gen_name) const {
    return create(gen_name, BuildArgs{}...);
  }

private:
  std::unordered_map<KeyT, build_function> builders_;
};

template <typename KeyT, typename ReturnType>
class CommonFactory<KeyT, ReturnType> {
public:
  using build_function = std::function<ReturnType()>;

  void register_builder(KeyT gen_name, build_function builder) {
    builders_.emplace(std::move(gen_name), std::move(builder));
  }

  auto create(const KeyT &gen_name) const {
    try {
      return builders_.at(gen_name)();
    } catch (std::out_of_range &e) {
      auto to_string = [](const auto &v) {
        if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
          return v;
        } else {
          return std::to_string(v);
        }
      };

      static thread_local std::string emsg =
          "Unable to create generator with name " + to_string(gen_name) +
          ".Builder not found.";
      throw std::runtime_error(emsg.c_str());
    }
  }

private:
  std::unordered_map<KeyT, build_function> builders_;
};

#endif // SLS_COMMON_FACTORY_H
