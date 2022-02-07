#pragma once

#include "variant_union.h"

/**
 * Base class for satisfying destructor triviality
 *
 * @tparam trivial - is variant trivially destructible
 */
template <bool trivial, typename First, typename... Other>
struct variant_storage {

  constexpr variant_storage() noexcept(std::is_nothrow_default_constructible_v<First>) requires(std::is_default_constructible_v<First>) = default;
  constexpr variant_storage() noexcept requires(!std::is_default_constructible_v<First>) : v_union(in_place_index<1 + sizeof...(Other)>) {}

   template <size_t I, typename... Args>
  constexpr variant_storage(in_place_index_t<I> index, Args&&... args) : v_union(index, std::forward<Args>(args)...) {}

   constexpr ~variant_storage() noexcept {
     auto& var = variant<First, Other...>::from_base(*this);
     if (!var.valueless_by_exception()) {
       var.reset();
     }
   }

  variant_union<false, First, Other...> v_union{};
};

template <typename First, typename... Other>
struct variant_storage<true, First, Other...> {

  constexpr variant_storage() noexcept(std::is_nothrow_default_constructible_v<First>) requires(std::is_default_constructible_v<First>) = default;
  constexpr variant_storage() noexcept requires(!std::is_default_constructible_v<First>) : v_union(in_place_index<1 + sizeof...(Other)>) {};

  template <size_t I, typename... Args>
  constexpr variant_storage(in_place_index_t<I> index, Args&&... args) : v_union(index, std::forward<Args>(args)...) {}

  constexpr ~variant_storage() noexcept = default;

  variant_union<true, First, Other...> v_union;
};
