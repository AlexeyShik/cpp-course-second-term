#include "variant_base_functions.h"

#pragma once

namespace details {
template <typename... Types>
concept TriviallyDestructible = (std::is_trivially_destructible_v<Types> && ...);

template <typename... Types>
concept CopyConstructible = (std::is_copy_constructible_v<Types> && ...);

template <typename... Types>
concept TriviallyCopyConstructible = CopyConstructible<Types...> && (std::is_trivially_copy_constructible_v<Types> && ...);

template <typename... Types>
concept CopyAssignable = (std::is_copy_assignable_v<Types> && ...);

template <typename... Types>
concept TriviallyCopyAssignable = CopyAssignable<Types...> && (std::is_trivially_copy_assignable_v<Types> && ...);

template <typename... Types>
concept CopyConstructibleVariant = CopyConstructible<Types...>&& CopyAssignable<Types...>;

template <typename... Types>
concept TriviallyCopyConstructibleVariant = TriviallyCopyConstructible<Types...>&& TriviallyCopyAssignable<Types...>&& TriviallyDestructible<Types...>;

template <typename... Types>
concept MoveConstructible = (std::is_move_constructible_v<Types> && ...);

template <typename... Types>
concept Swappable = (std::is_swappable_v<Types> && ...);

template <typename... Types>
concept NothrowSwappable = (std::is_nothrow_swappable_v<Types> && ...);

template <typename... Types>
concept MoveAssignable = (std::is_move_assignable_v<Types> && ...);

template <typename... Types>
concept NothrowMoveConstructible = MoveConstructible<Types...> && (std::is_nothrow_move_constructible_v<Types> && ...);

template <typename T, typename Tj>
concept NothrowConstructibleInConvertingAssigment = !NothrowMoveConstructible<Tj> && std::is_nothrow_constructible_v<Tj, T>;

template <typename... Types>
concept NothrowMoveAssignable = MoveAssignable<Types...> && (std::is_nothrow_move_assignable_v<Types> && ...);

template <typename... Types>
concept TriviallyMoveConstructible = NothrowMoveConstructible<Types...> && (std::is_trivially_move_constructible_v<Types> && ...);

template <typename... Types>
concept TriviallyMoveAssignable = NothrowMoveAssignable<Types...> && (std::is_trivially_move_assignable_v<Types> && ...);

template <typename... Types>
concept MoveConstructibleVariant = MoveConstructible<Types...> && MoveAssignable<Types...>;

template <typename... Types>
concept TriviallyMoveConstructibleVariant = TriviallyMoveConstructible<Types...> && TriviallyMoveAssignable<Types...> && TriviallyDestructible<Types...>;

template <typename T, typename F>
concept ValidCreation = requires(T&& t) {
  F{std::forward<T>(t)};
};

template <typename T, typename F>
concept ValidArrayCreation = ValidCreation<T, F[]>;

template <typename T, typename... Types>
concept OccursOnce = details::count_occurrences_v<T, Types...> == 1;

template <size_t I, typename... Types>
concept InBounds = I <= variant_size_v<variant<Types...>>;

template <typename... Types>
concept ValidSequence = details::count_occurrences_v<std::integral_constant<size_t, static_cast<size_t>(0)>, Types...> == 0;
};
