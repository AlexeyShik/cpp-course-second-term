#pragma once

#include "variant_concepts.h"

template<bool trivial, typename... Types>
union variant_union;

/**
 * In copy/move constructors I need to initialize base class firstly,
 * so I would initialize only this base and no constructors of <First, Other...> would be called
 */
template<bool trivial>
union variant_union<trivial> {

  constexpr variant_union() = default;
  constexpr variant_union(in_place_index_t<0>) {};
};

template<typename First, typename... Other>
union variant_union<false, First, Other...> {

  constexpr variant_union() noexcept(std::is_nothrow_default_constructible_v<First>) requires(std::is_default_constructible_v<First>) : first() {}
  constexpr variant_union() noexcept : other() {}

  template <size_t I, typename... Args>
  constexpr variant_union(in_place_index_t<I>, Args&&... args) : other(in_place_index<I - 1>, std::forward<Args>(args)...) {}

  template <typename... Args>
  constexpr variant_union(in_place_index_t<0>, Args&&... args) : first(std::forward<Args>(args)...) {}

  constexpr ~variant_union() noexcept {};

  template <size_t I>
  constexpr void reset(in_place_index_t<I>) {
    other.reset(in_place_index<I - 1>);
  }

  constexpr void reset(in_place_index_t<0>) {
    first.~First();
  }

  template <size_t I>
  constexpr auto&& get(in_place_index_t<I>) {
    return other.template get(in_place_index<I - 1>);
  }

  template <size_t I = 0>
  constexpr auto&& get(in_place_index_t<0>) {
    return first;
  }

  template <size_t I>
  constexpr const auto&& get(in_place_index_t<I>) const {
    return other.template get(in_place_index<I - 1>);
  }

  template <size_t I = 0>
  constexpr const auto&& get(in_place_index_t<0>) const {
    return first;
  }

  template <typename... Args>
  First& emplace(in_place_index_t<0>, Args&&... args) {
    new (const_cast<std::remove_const_t<First>*>(std::addressof(first))) std::remove_cvref_t<First>(std::forward<Args>(args)...);
    return const_cast<std::remove_reference_t<First>&>(first);
  }

  template <size_t I, typename... Args>
  auto& emplace(in_place_index_t<I>, Args&&... args) {
    return other.template emplace(in_place_index<I - 1>, std::forward<Args>(args)...);
  }

  template <typename T>
  constexpr void assign(in_place_index_t<0>, T&& value) {
    const_cast<std::remove_const_t<First>&>(first) = std::forward<T>(value);
  }

  template <size_t I, typename T>
  constexpr void assign(in_place_index_t<I>, T&& value) {
    other.template assign(in_place_index<I - 1>, std::forward<T>(value));
  }

  First first;
  variant_union<false, Other...> other;
};


template <typename First, typename... Other>
union variant_union<true, First, Other...> {

  constexpr variant_union() noexcept(std::is_nothrow_default_constructible_v<First>) requires(std::is_default_constructible_v<First>) : first() {};
  constexpr variant_union() noexcept : other() {};

  template <size_t I, typename... Args>
  constexpr variant_union(in_place_index_t<I>, Args&&... args) : other(in_place_index<I - 1>, std::forward<Args>(args)...) {}

  template <typename... Args>
  constexpr variant_union(in_place_index_t<0>, Args&&... args) : first(std::forward<Args>(args)...) {}

  template <size_t I>
  constexpr auto&& get(in_place_index_t<I>) {
    return other.template get(in_place_index<I - 1>);
  }

  template <size_t I = 0>
  constexpr auto&& get(in_place_index_t<0>) {
    return first;
  }

  template <size_t I>
  constexpr const auto&& get(in_place_index_t<I>) const {
    return other.template get(in_place_index<I - 1>);
  }

  template <size_t I = 0>
  constexpr const auto&& get(in_place_index_t<0>) const {
    return first;
  }

  template <typename... Args>
  First& emplace(in_place_index_t<0>, Args&&... args) {
    new (const_cast<std::remove_const_t<First>*>(std::addressof(first))) std::remove_cvref_t<First>(std::forward<Args>(args)...);
    return const_cast<std::remove_reference_t<First>&>(first);
  }

  template <size_t I, typename... Args>
  auto& emplace(in_place_index_t<I>, Args&&... args) {
    return other.template emplace(in_place_index<I - 1>, std::forward<Args>(args)...);
  }

  template <typename T>
  constexpr void assign(in_place_index_t<0>, T&& value) {
    const_cast<std::remove_const_t<First>&>(first) = std::forward<T>(value);
  }

  template <size_t I, typename T>
  constexpr void assign(in_place_index_t<I>, T&& value) {
    other.template assign(in_place_index<I - 1>, std::forward<T>(value));
  }

  template <size_t I>
  constexpr void reset(in_place_index_t<I>) {
    other.reset(in_place_index<I - 1>);
  }

  constexpr void reset(in_place_index_t<0>) {
    first.~First();
  }

  First first;
  variant_union<true, Other...> other;
};
