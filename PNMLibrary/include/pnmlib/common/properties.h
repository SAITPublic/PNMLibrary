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

#ifndef PNM_PROPERTIES_H
#define PNM_PROPERTIES_H

#include "pnmlib/common/compiler.h"

#include <any>
#include <cstdint>
#include <unordered_map>

namespace pnm::property {
/** @brief The definition of the supported properties */
// [TODO: y-lavrinenko] Think about property type definition by property class
enum class PropertiesTypes : uint8_t { MEM_ALLOCATION_POLICY, MEM_LOCATION };

/** @brief Base class for a property */
template <PropertiesTypes Type> struct PNM_API Base {
  static constexpr auto type = Type;

protected:
  // Make inheritance safe and be sure that we will not destroy object via
  // delete (Base*)ptr;
  ~Base() = default;
};

template <typename T> struct PropertiesTraits {
  static constexpr PropertiesTypes type = T::type;
};

/** @brief Class to handle the list of unique properties */
class PNM_API PropertiesList {
public:
  template <typename T> PropertiesList(T prop) {
    add_property(std::move(prop));
  }

  template <typename... Props> PropertiesList(Props... prop) {
    (add_property(std::move(prop)), ...);
  }

  template <typename T> void add_property(T prop) {
    value_props_[PropertiesTraits<T>::type] = std::move(prop);
  }

  template <typename T> bool has_property() const {
    return value_props_.count(PropertiesTraits<T>::type);
  }

  template <typename T> T get_property() const {
    return std::any_cast<T>(value_props_.at(PropertiesTraits<T>::type));
  }

private:
  std::unordered_map<PropertiesTypes, std::any> value_props_;
};

/** @param Overload of pipe operator to support properties list generation in
 * C-style way
 *
 * With this construction we can create PropertiesList object in next way:
 *
 * @code
 * PropertiesList prop = PropA() | PropB() | PropC(42) | PropD(10, 12)
 * @endcode
 *
 */
template <typename T> PropertiesList operator|(PropertiesList list, T props) {
  list.add_property(std::move(props));
  return list;
}

} // namespace pnm::property

#endif // PNM_PROPERTIES_H
