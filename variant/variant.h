#pragma once

#include "variant_storage.h"
#include "variant_visitor.h"
#include <cassert>

namespace details {
template <size_t I, typename T, typename... Types>
struct conversion_functions;

template <size_t I, typename T, typename First, typename... Other>
struct conversion_functions<I, T, First, Other...> : conversion_functions<I - 1, T, Other...> {
  static std::integral_constant<size_t, I> func(First) requires(ValidArrayCreation<T, First>);
  static void func() requires(!ValidArrayCreation<T, First>);
  using conversion_functions<I - 1, T, Other...>::func;
};

template <typename T, typename... Types>
struct conversion_functions<0, T, Types...> {
  static void func();
};

template <typename T, typename... Types>
using type_for_conversion =
    decltype(conversion_functions<sizeof...(Types), T, Types...>::func(std::declval<T>()));
};

template <typename First, typename... Other>
struct variant<First, Other...> : private variant_storage<details::TriviallyDestructible<First, Other...>, First, Other...> {
  using base = variant_storage<details::TriviallyDestructible<First, Other...>, First, Other...>;

private:
  static constexpr size_t remain_steps_from_index(size_t i) {
    return (sizeof...(Other)) - i + 1;
  }

public:

  constexpr variant() noexcept(std::is_nothrow_default_constructible_v<First>) requires(std::is_default_constructible_v<First>) = default;
  constexpr variant(const variant&) requires(details::TriviallyCopyConstructible<First, Other...>) = default;
  constexpr variant(const variant& other) requires(details::CopyConstructible<First, Other...>) {
    emplace_variant(other);
  }

  constexpr variant(variant&&) noexcept requires(details::TriviallyMoveConstructible<First, Other...>) = default;
  constexpr variant(variant&& other) noexcept(details::NothrowMoveConstructible<First, Other...>) requires(details::MoveConstructible<First, Other...>) {
    emplace_variant(std::move(other));
  }

  constexpr variant &operator=(variant const&) requires(details::TriviallyCopyConstructibleVariant<First, Other...>) = default;
  constexpr variant &operator=(variant const& other) requires(details::CopyConstructibleVariant<First, Other...>) {
    if (this != &other) {
      if (other.valueless_by_exception()) {
        if (!valueless_by_exception()) {
          reset();
        }
      } else {
        visitor::visit_index<void>([this, &other] (auto index1, auto index2) {
          if constexpr (index1() == index2()) {
            auto& left = const_cast<variant_alternative_t<index1(), variant<First, Other...>>&>(get<index1()>(*this));
            left = get<index2()>(other);
          } else if (std::is_nothrow_copy_constructible_v<decltype(get<index2()>(other))> || !std::is_nothrow_move_constructible_v<decltype(get<index2()>(other))>) {
            emplace<index2()>(get<index2()>(other));
            index_ = other.index();
          } else {
            this->operator=(variant(other));
          }
        }, *this, other);
      }
    }
    return *this;
  };

  constexpr variant &operator=(variant&&) noexcept requires(details::TriviallyMoveConstructibleVariant<First, Other...>) = default;
  constexpr variant &operator=(variant&& other) noexcept(details::NothrowMoveAssignable<First, Other...> && details::NothrowMoveConstructible<First, Other...>)
      requires(details::MoveConstructibleVariant<First, Other...>) {
    if (this != &other) {
      if (other.valueless_by_exception()) {
        if (!valueless_by_exception()) {
          reset();
        }
      } else {
        visitor::visit_index<void>([this, &other] (auto index1, auto index2) {
          if constexpr (index1() == index2()) {
            auto& left = const_cast<variant_alternative_t<index1(), variant<First, Other...>>&>(get<index1()>(*this));
            left = std::move(get<index2()>(std::move(other)));
          } else {
            emplace<index2()>(get<index2()>(std::move(other)));
            index_ = other.index();
          }
        }, *this, std::move(other));
      }
    }
    return *this;
  };

  template <typename T,
            size_t rev_index_for_conversion = details::type_for_conversion<T, First, Other...>(),
            size_t index_for_conversion = remain_steps_from_index(rev_index_for_conversion),
            typename Tj = variant_alternative_t<index_for_conversion, variant<First, Other...>>>
  constexpr variant(T&& value) noexcept(std::is_nothrow_constructible_v<Tj, T>)
      requires(!details::is_in_place_v<std::remove_cvref_t<T>> && !std::is_same_v<variant<First, Other...>, std::remove_cvref_t<T>> && index_for_conversion <= sizeof...(Other) && std::is_constructible_v<Tj, T>)
      : base(in_place_index<index_for_conversion>, std::forward<T>(value)), index_(index_for_conversion) {}

  template<typename T, typename... Args, size_t index = details::index_of_v<T, First, Other...>> requires(std::is_constructible_v<T, Args...> && details::OccursOnce<T, First, Other...>)
  constexpr variant(in_place_type_t<T>, Args&&... args) : base(in_place_index<index>, std::forward<Args>(args)...), index_(index) {}

  template <size_t I, typename... Args> requires(details::InBounds<I, First, Other...> && std::is_constructible_v<variant_alternative_t<I, variant<First, Other...>>, Args...>)
  constexpr variant(in_place_index_t<I> index, Args&&... args) : base(index, std::forward<Args>(args)...), index_(I) {}

  template <typename T,
      size_t rev_index_for_conversion = details::type_for_conversion<T, First, Other...>(),
      size_t index_for_conversion = remain_steps_from_index(rev_index_for_conversion),
      typename Tj = variant_alternative_t<index_for_conversion, variant<First, Other...>>>
  constexpr variant& operator=(T &&value) noexcept(std::is_nothrow_assignable_v<Tj, T> && std::is_nothrow_constructible_v<Tj, T>)
      requires(!std::is_same_v<variant<First, Other...>, std::remove_cvref_t<T>> && std::is_assignable_v<Tj&, T> && std::is_constructible_v<Tj, T>) {
    if (index() == index_for_conversion) {
      assign<index_for_conversion>(std::forward<T>(value));
    } else if (details::NothrowConstructibleInConvertingAssigment<Tj, T>) {
      emplace<index_for_conversion>(std::forward<T>(value));
    } else {
      emplace<index_for_conversion>(Tj(std::forward<T>(value)));
    }
    return *this;
  }

  constexpr void swap(variant& other) noexcept(details::NothrowMoveConstructible<First, Other...> && details::NothrowSwappable<First, Other...>)
      requires(details::MoveConstructible<First, Other...> && details::Swappable<First, Other...>) {
    if (valueless_by_exception() && other.valueless_by_exception()) {
      return;
    } else if (other.valueless_by_exception()) {
      other.emplace_variant(std::move(*this));
      reset();
    } else if (valueless_by_exception()) {
      emplace_variant(std::move(other));
      other.reset();
    } else if (index() == other.index()) {
      visitor::visit_index<void>([this, &other] (auto index) {
        variant_alternative_t<index, variant<First, Other...>>& left = const_cast<variant_alternative_t<index, variant<First, Other...>>&>(get<index>(*this));
        variant_alternative_t<index, variant<First, Other...>>& right = const_cast<variant_alternative_t<index, variant<First, Other...>>&>(get<index>(other));

        using std::swap;
        swap(left, right);
        swap(index_, other.index_);
      }, *this);
    } else {
      std::swap(*this, other);
    }
  }

  template <size_t I, typename... Args> requires(details::InBounds<I, First, Other...>)
  constexpr variant_alternative_t<I, variant>& emplace(Args&&... args) {
    if (!valueless_by_exception()) {
      reset();
    }
    auto& value = base::v_union.emplace(in_place_index<I>, std::forward<Args>(args)...);
    index_ = I;
    return value;
  }

  template <typename T, typename... Args>
  T& emplace(Args&&... args) {
    return emplace<details::index_of_v<T, First, Other...>>(std::forward<Args>(args)...);
  }

  constexpr ~variant() noexcept = default;

  constexpr size_t index() const noexcept {
    return index_;
  }

  constexpr bool valueless_by_exception() const noexcept {
    return index() == variant_npos;
  }

  static constexpr typename variant<First, Other...>::base& to_base(const variant<First, Other...> &var) noexcept {
    return const_cast<typename variant<First, Other...>::base&>(static_cast<const typename variant<First, Other...>::base&>(var));
  }

  static constexpr typename variant<First, Other...>::base&& to_base(variant<First, Other...> &&var) noexcept {
    return static_cast<typename variant<First, Other...>::base&&>(std::move(var));
  }

  static constexpr variant<First, Other...>& from_base(const variant_storage<details::TriviallyDestructible<First, Other...>, First, Other...> &base) noexcept {
    return const_cast<variant<First, Other...>&>(static_cast<const variant<First, Other...>&>(base));
  }

  void reset() {
    visitor::visit_index<void>([this] (auto index) {
      this->v_union.reset(in_place_index<index()>);
    }, *this);
    index_ = variant_npos;
  }

private:

  void emplace_variant(const variant& other) {
    visitor::visit_index<void>([this, &other] (auto index) {
      using type = typename std::remove_cvref_t<decltype(get<index()>(other))>;
      emplace<index()>(std::forward<const type>(get<type>(other)));
    }, other);
    index_ = other.index();
  }

  void emplace_variant(variant&& other) {
    visitor::visit_index<void>([this, &other] (auto index) {
      using type = typename std::remove_cvref_t<decltype(get<index()>(std::move(other)))>;
      emplace<index()>(std::move(get<type>(std::move(other))));
    }, std::move(other));
    index_ = other.index();
  }

  template <size_t I, typename T>
  constexpr void assign(T&& value) {
    try {
      to_base(*this).v_union.assign(in_place_index<I>, std::forward<T>(value));
    } catch (...) {
      index_ = variant_npos;
      throw;
    }
  }

  size_t index_{0};
};

template <typename T, typename... Types>
constexpr bool holds_alternative(const variant<Types...>& var) noexcept {
    return var.index() == details::index_of_v<T, Types...>;
}

template <size_t I, typename... Types>
constexpr variant_alternative_t<I, variant<Types...>>& get(variant<Types...>& var) {
  if (var.index() != I) {
    throw bad_variant_access();
  }
  return const_cast<variant_alternative_t<I, variant<Types...>>&>(variant<Types...>::to_base(var).v_union.template get(in_place_index<I>));
}

template <size_t I, typename... Types>
constexpr const variant_alternative_t<I, variant<Types...>>& get(const variant<Types...>& var) {
  return get<I, Types...>(const_cast<variant<Types...>&>(var));
}

template <size_t I, typename... Types>
constexpr variant_alternative_t<I, variant<Types...>>&& get(variant<Types...>&& var) {
  return std::move(get<I>(var));
}

template <size_t I, typename... Types>
constexpr const variant_alternative_t<I, variant<Types...>>&& get(const variant<Types...>&& var) {
  return get<I, Types...>(const_cast<variant<Types...>&&>(std::move(var)));
}

template <typename T, typename... Types>
using variant_alternative_t_types = variant_alternative_t<details::index_of_v<T, Types...>, variant<Types...>>;

template <typename T, typename... Types>
constexpr const variant_alternative_t_types<T, Types...>& get(const variant<Types...>& var) {
  return get<details::index_of_v<T, Types...>>(var);
}

template <typename T, typename... Types>
constexpr variant_alternative_t_types<T, Types...>&& get(variant<Types...>&& var) {
  return get<details::index_of_v<T, Types...>>(std::move(var));
}

template <typename T, typename...Types>
constexpr T* get_if(const variant<Types...> *var) {
  return get_if<details::index_of_v<T, Types...>>(var);
}

template <size_t I, typename...Types>
constexpr variant_alternative_t<I, variant<Types...>>* get_if(const variant<Types...> *var) {
  if (var == nullptr || var->index() != I) {
    return nullptr;
  }
  return const_cast<variant_alternative_t<I, variant<Types...>>*>(std::addressof(get<I>(*var)));
}

template <typename... Types>
constexpr bool operator==(const variant<Types...> &v1, const variant<Types...> &v2) noexcept {
  if (v1.valueless_by_exception() && v2.valueless_by_exception()) {
    return true;
  }
  if (v1.valueless_by_exception() || v2.valueless_by_exception()) {
    return false;
  }
  return visitor::visit_index<bool>([&v1, &v2] (auto index1, auto index2) {
    if constexpr (index1() != index2()) {
      return false;
    } else {
      return get<index1()>(v1) == get<index2()>(v2);
    }
  }, v1, v2);
}

template <typename... Types>
constexpr bool operator!=(const variant<Types...> &v1, const variant<Types...> &v2) noexcept {
  return !(v1 == v2);
}

template <typename... Types>
constexpr bool operator<(const variant<Types...> &v1, const variant<Types...> &v2) noexcept {
  if (v2.valueless_by_exception()) {
    return false;
  }
  if (v1.valueless_by_exception()) {
    return true;
  }
  return visitor::visit_index<bool>([&v1, &v2] (auto index1, auto index2) {
    if constexpr (index1() < index2()) {
      return true;
    } else if constexpr (index1() > index2()) {
      return false;
    } else {
      return get<index1()>(v1) < get<index2()>(v2);
    }
  }, v1, v2);
}

template <typename... Types>
constexpr bool operator<=(const variant<Types...> &v1, const variant<Types...> &v2) noexcept {
  return v1 < v2 || v1 == v2;
}

template <typename... Types>
constexpr bool operator>(const variant<Types...> &v1, const variant<Types...> &v2) noexcept {
  return !(v1 <= v2);
}

template <typename... Types>
constexpr bool operator>=(const variant<Types...> &v1, const variant<Types...> &v2) noexcept {
  return !(v1 < v2);
}
