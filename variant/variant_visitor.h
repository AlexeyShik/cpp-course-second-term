#include "variant_concepts.h"
#include <array>
#include <algorithm>

#pragma once

namespace visitor {

template <typename... Types>
constexpr std::array<typename std::common_type<Types...>::type, sizeof...(Types)> make_array(Types&&... t) {
  return {std::forward<Types>(t)...};
}

template <typename F, typename... Vs, std::size_t... Is>
constexpr auto make_fmatrix_impl(std::index_sequence<Is...>) {
  if constexpr (details::ValidSequence<std::integral_constant<size_t, Is>...>) {
    return [] (F&& f, Vs&&... vs) { return std::forward<F>(f)(get<Is - 1>(std::forward<Vs>(vs))...); };
  } else {
    return [] (F&&, Vs&&... vs) -> std::invoke_result_t<F, decltype(get<0>(std::forward<Vs>(vs)))...> { throw bad_variant_access(); };
  }
}

template <typename F, typename... Vs, std::size_t... Is, std::size_t... Js, typename... Ls>
constexpr auto make_fmatrix_impl(std::index_sequence<Is...>, std::index_sequence<Js...>, Ls... ls) {
  return make_array(make_fmatrix_impl<F, Vs...>(std::index_sequence<Is..., Js>{}, ls...)...);
}

template <typename F, typename... Vs>
constexpr auto make_fmatrix() {
  return make_fmatrix_impl<F, Vs...>(std::index_sequence<>{},
                                     std::make_index_sequence<1 + variant_size_v<std::remove_cvref_t<Vs>>>{}...);
}

template <typename R, typename F, typename... Vs, std::size_t... Is>
constexpr auto make_fmatrix_index_impl(std::index_sequence<Is...>) {
  if constexpr (details::ValidSequence<std::integral_constant<size_t, Is>...>) {
    return [] (F&& f) { return std::forward<F>(f)(std::integral_constant<size_t, Is - 1>()...); };
  } else {
    return [] (F&&) -> R { throw bad_variant_access(); };
  }
}

template <typename R, typename F, std::size_t... Is, std::size_t... Js, typename... Ls>
constexpr auto make_fmatrix_index_impl(std::index_sequence<Is...>, std::index_sequence<Js...>, Ls... ls) {
  return make_array(make_fmatrix_index_impl<R, F>(std::index_sequence<Is..., Js>{}, ls...)...);
}

template <typename R, typename F, typename... Vs>
constexpr auto make_fmatrix_index() {
  return make_fmatrix_index_impl<R, F>(std::index_sequence<>{},
                                     std::make_index_sequence<1 + variant_size_v<std::remove_cvref_t<Vs>>>{}...);
}

template <typename T>
constexpr T&& at(T&& elem) {
  return std::forward<T>(elem);
}

template <typename T, typename... Is>
constexpr auto&& at(T&& elems, std::size_t i, Is... is) {
  return at(std::forward<T>(elems)[i], is...);
}

template <typename F, typename... Vs>
constexpr inline auto fmatrix = make_fmatrix<F&&, Vs&&...>();

template <typename R, typename F, typename... Vs>
constexpr inline auto fmatrix_index = make_fmatrix_index<R, F&&, Vs&&...>();

template <typename R, typename F, typename... Vs>
constexpr R visit_index(F&& f, Vs&&... vs) {
  return at(fmatrix_index<R, F, Vs...>, (vs.index() + 1)...)(std::forward<F>(f));
}
};

template <typename F, typename... Vs>
constexpr decltype(auto) visit(F&& f, Vs&&... vs) {
  return visitor::at(visitor::fmatrix<F, Vs...>, (vs.index() + 1)...)(std::forward<F>(f), std::forward<Vs>(vs)...);
}
