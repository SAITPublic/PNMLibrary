/*
 *  Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 *  This software is proprietary of Samsung Electronics.
 *  No part of this software, either material or conceptual may be copied or
 *  distributed, transmitted, transcribed, stored in a retrieval system or
 *  translated into any human or computer language in any form by any means,
 *  electronic, mechanical, manual or otherwise, or disclosed to third parties
 *  without the express written permission of Samsung Electronics.
 */

#ifndef PNM_DISPATCHER_H
#define PNM_DISPATCHER_H

#include <type_traits>
#include <typeindex>
#include <unordered_map>

namespace pnm {

class Context;

class Dispatcher {
public:
  template <typename T> using internal_function = void (*)(T *, pnm::Context *);

  template <typename Operator>
  void register_function(internal_function<Operator> func) {
    functions_[std::type_index(typeid(std::decay_t<Operator>))] =
        reinterpret_cast<invoke_function>(func);
  }

  template <typename Operator>
  void invoke(Operator &op, pnm::Context *ctx) const {
    const auto &func =
        functions_.at(std::type_index(typeid(std::decay_t<Operator>)));
    reinterpret_cast<internal_function<Operator>>(func)(&op, ctx);
  }

private:
  using invoke_function = void (*)();

  std::unordered_map<std::type_index, invoke_function> functions_;
};

} // namespace pnm

#endif // PNM_DISPATCHER_H
