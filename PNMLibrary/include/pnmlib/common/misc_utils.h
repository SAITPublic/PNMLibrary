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

#ifndef PNM_MISC_UTILS_H
#define PNM_MISC_UTILS_H

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace pnm::utils {

template <typename T> inline auto as_vatomic(volatile void *value) {
  return reinterpret_cast<volatile std::atomic<T> *>(value);
}

template <typename C, typename T = typename C::value_type>
size_t byte_size_of_container(const C &container) {
  return sizeof(T) * container.size();
}

/** @brief Return the aligned `value` as specified by `alignment`
 *
 * @param value
 * @param alignment Requested alignment, in bytes. Should be power of 2
 * @return the aligned value AV that AV % alignment == 0, AV >= value
 */
constexpr uint64_t fast_align_up(uint64_t value, uint64_t alignment) {
  assert((alignment != 0 && (alignment & (alignment - 1)) == 0) &&
         "The alignment is not a power of 2");
  return (value + alignment - 1) & ~(alignment - 1);
}

/** @brief A container for 2 types
 */
template <typename First, typename Second> struct TypePair {
  using first = First;
  using second = Second;
};

/** @brief Given type T that is equal to either First or Second, returns the
 * other type
 */
template <typename T, typename First, typename Second>
using OtherType = std::conditional_t<std::is_same_v<T, First>, Second, First>;

template <typename... Types> struct TypeTuple {};

namespace details {

template <typename NewType, typename... Types>
constexpr auto prepend_type(NewType /*unused*/,
                            TypeTuple<Types...> /*unused*/) {
  return TypeTuple<NewType, Types...>{};
}

template <typename NewType, typename Tuple>
using prepend_type_t =
    decltype(prepend_type(std::declval<NewType>(), std::declval<Tuple>()));

template <typename... Types1, typename... Types2>
constexpr auto concat_type_tuples(TypeTuple<Types1...> /*unused*/,
                                  TypeTuple<Types2...> /*unused*/) {
  return TypeTuple<Types1..., Types2...>{};
}

template <typename Tuple1, typename Tuple2>
using concat_type_tuples_t = decltype(concat_type_tuples(
    std::declval<Tuple1>(), std::declval<Tuple2>()));

template <typename NewType>
constexpr auto prepend_type_to_all(NewType /*unused*/, TypeTuple<> /*unused*/) {
  return TypeTuple<>{};
}

template <typename NewType, typename FirstTuple, typename... OtherTuples>
constexpr auto
prepend_type_to_all(NewType /*unused*/,
                    TypeTuple<FirstTuple, OtherTuples...> /*unused*/) {
  using prepended_first = TypeTuple<prepend_type_t<NewType, FirstTuple>>;
  using prepended_others = decltype(prepend_type_to_all(
      std::declval<NewType>(), std::declval<TypeTuple<OtherTuples...>>()));
  return concat_type_tuples_t<prepended_first, prepended_others>{};
}

template <typename NewType, typename TupleOfTuples>
using prepend_type_to_all_t = decltype(prepend_type_to_all(
    std::declval<NewType>(), std::declval<TupleOfTuples>()));

template <typename TupleOfTuples>
constexpr auto prepend_tuple_to_all(TypeTuple<> /*unused*/,
                                    TupleOfTuples /*unused*/) {
  return TupleOfTuples{};
}

template <typename FirstType, typename TupleOfTuples>
constexpr auto prepend_tuple_to_all(TypeTuple<FirstType> /*unused*/,
                                    TupleOfTuples /*unused*/) {
  return prepend_type_to_all_t<FirstType, TupleOfTuples>{};
}

// SFINAE to exclude instantiation with empty OtherTypes.
// (Occur only in clang < 16.0)
template <typename FirstType, typename... OtherTypes, typename TupleOfTuples,
          typename = std::enable_if_t<sizeof...(OtherTypes) != 0>>
constexpr auto
prepend_tuple_to_all(TypeTuple<FirstType, OtherTypes...> /*unused*/,
                     TupleOfTuples /*unused*/) {
  using prepended_first = prepend_type_to_all_t<FirstType, TupleOfTuples>;
  using prepended_others = decltype(prepend_tuple_to_all(
      std::declval<TypeTuple<OtherTypes...>>(), std::declval<TupleOfTuples>()));
  return concat_type_tuples_t<prepended_first, prepended_others>{};
}

template <typename AddedTuple, typename TupleOfTuples>
using prepend_tuple_to_all_t = decltype(prepend_tuple_to_all(
    std::declval<AddedTuple>(), std::declval<TupleOfTuples>()));

constexpr auto tuple_product() { return TypeTuple<TypeTuple<>>{}; }

template <typename First, typename... Others>
constexpr auto tuple_product(First first, Others... others) {
  auto product_of_others = tuple_product(others...);
  return prepend_tuple_to_all(first, product_of_others);
}

} // namespace details

template <typename... Tuples>
using tuple_product_t =
    decltype(details::tuple_product(std::declval<Tuples>()...));

static_assert(std::is_same_v<
              TypeTuple<TypeTuple<int, float>, TypeTuple<int, double>,
                        TypeTuple<long, float>, TypeTuple<long, double>>,
              tuple_product_t<TypeTuple<int, long>, TypeTuple<float, double>>>);

/**@brief Return true if one and only one element of the range [begin, sentinel)
 * satisfies predicate p
 */
template <typename It, typename Sentinel, typename Predicate>
bool one_of(It begin, Sentinel end, Predicate p) {
  return std::count_if(begin, end, p) == 1;
}

namespace visitor {
// [TODO: @b.palkin] Unify with the same pattern in accessor_builder.cpp
/** @brief Overload pattern for making visitors for std::visit out of
 * сallable objects.
 */
template <class... Ts> struct overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overload(Ts...) -> overload<Ts...>;
} // namespace visitor

} // namespace pnm::utils

#endif // PNM_MISC_UTILS_H
