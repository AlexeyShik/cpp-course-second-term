#pragma once

struct bad_variant_access : std::exception {
  constexpr const char* what() const noexcept override {
    return "bad_variant_access";
  }
};

template <typename... Types>
struct variant;

template <size_t I>
struct in_place_index_t {};

template <size_t I>
inline constexpr in_place_index_t<I> in_place_index{};

template <typename T>
struct in_place_type_t {};

template <typename T>
inline constexpr in_place_type_t<T> in_place_type{};

template <typename T>
struct variant_size;

template <typename T>
inline constexpr size_t variant_size_v = variant_size<T>::value;

template <typename T>
struct variant_size<const T> : variant_size<T> {};

template <typename... Types>
struct variant_size<variant<Types...>> : std::integral_constant<size_t, sizeof...(Types)> {};

template <size_t I, typename T>
struct variant_alternative;

template <size_t I, typename T>
using variant_alternative_t = typename variant_alternative<I, T>::type;

template <size_t I, typename First, typename... Other>
struct variant_alternative<I, variant<First, Other...>> {
  using type = typename variant_alternative<I - 1, variant<Other...>>::type;
};

template <typename First, typename... Other>
struct variant_alternative<0, variant<First, Other...>> {
  using type = First;
};

template <size_t I, typename... Types>
struct variant_alternative<I, const variant<Types...>> {
  using type = const typename variant_alternative<I, variant<Types...>>::type;
};

inline constexpr std::size_t variant_npos = -1;

namespace details {
template <typename T, typename... Types>
struct index_of;

template <typename T, typename... Types>
inline constexpr size_t index_of_v = index_of<T, Types...>::value;

template <typename T>
struct index_of<T> : std::integral_constant<size_t, variant_npos> {};

template <typename T, typename First, typename... Other>
struct index_of<T, First, Other...>
    : std::integral_constant<size_t, std::is_same_v<std::remove_cvref<T>, std::remove_cvref<First>>
                                         ? 0
                                         : 1 + index_of_v<T, Other...>> {};

template <typename T, typename... Types>
struct count_occurrences;

template <typename T, typename... Types>
inline constexpr size_t count_occurrences_v = count_occurrences<T, Types...>::value;

template <typename T, typename First>
struct count_occurrences<T, First> : std::integral_constant<size_t, std::is_same_v<T, First> ? 1 : 0> {};

template <typename T, typename First, typename... Other>
struct count_occurrences<T, First, Other...>
    : std::integral_constant<size_t, std::is_same_v<T, First> + count_occurrences_v<T, Other...>> {};

template <typename T>
struct is_in_place : std::integral_constant<size_t, false> {};

template <typename T>
struct is_in_place<in_place_type_t<T>> : std::integral_constant<size_t, true> {};

template <size_t I>
struct is_in_place<in_place_index_t<I>> : std::integral_constant<size_t, true> {};

template <typename T>
inline constexpr bool is_in_place_v = is_in_place<T>::value;
};
