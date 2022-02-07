#pragma once

#include <concepts>
#include <memory>

struct nullopt_t {};
inline constexpr nullopt_t nullopt{};

struct in_place_t {};
inline constexpr in_place_t in_place{};

namespace detail {

template <typename T, typename... Args>
concept Constructible = std::is_constructible_v<T, Args...>;

template <typename T>
concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

template <typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

template <typename T>
concept CopyConstructible = std::is_copy_constructible_v<T>;

template <typename T>
concept MoveConstructible = std::is_move_constructible_v<T>;

template <typename T>
concept CopyAssignable = std::is_copy_assignable_v<T>;

template <typename T>
concept MoveAssignable = std::is_move_assignable_v<T>;

struct dummy_t {};

template <typename T, bool trivial>
struct destructor_optional_base {

  bool active{false};
  union {
    dummy_t dummy;
    T value;
  };

  constexpr destructor_optional_base() noexcept : dummy(dummy_t()) {}

  template <typename... Args> requires Constructible<T, Args...>
  constexpr destructor_optional_base(in_place_t, Args... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
      : active{true}, value(std::forward<Args>(args)...) {}

  constexpr destructor_optional_base(const T& value_) noexcept
      : active{true}, value(value_) {}

  constexpr destructor_optional_base(T&& value_) noexcept
      : active{true}, value(std::move(value_)) {}

  ~destructor_optional_base() noexcept(std::is_nothrow_destructible_v<T>) {
    reset();
  }

  void reset() noexcept(std::is_nothrow_destructible_v<T>) {
    if (active) {
      value.~T();
      active = false;
    }
  }
};

template <typename T>
struct destructor_optional_base<T, true> {

  bool active{false};
  union {
    detail::dummy_t dummy;
    T value;
  };

  constexpr destructor_optional_base() noexcept : dummy(dummy_t()) {}

  template <typename... Args> requires Constructible<T, Args...>
  constexpr destructor_optional_base(in_place_t, Args... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
      : active{true}, value(std::forward<Args>(args)...) {}

  constexpr destructor_optional_base(const T& value_) noexcept
      : active{true}, value(value_) {}

  constexpr destructor_optional_base(T&& value_) noexcept
      : active{true}, value(std::move(value_)) {}

  ~destructor_optional_base() noexcept = default;

  constexpr void reset() noexcept {
    active = false;
  }
};

template <typename T>
struct copy_optional_base : destructor_optional_base<T, TriviallyDestructible<T>> {
  using base = destructor_optional_base<T, TriviallyDestructible<T>>;
  using base::base;

  constexpr copy_optional_base() noexcept = default;

  constexpr copy_optional_base(const copy_optional_base&) noexcept requires(TriviallyCopyable<T>) = default;
  constexpr copy_optional_base(copy_optional_base&&) noexcept requires(TriviallyCopyable<T>) = default;
  constexpr copy_optional_base& operator=(const copy_optional_base&) noexcept requires(TriviallyCopyable<T>) = default;
  constexpr copy_optional_base& operator=(copy_optional_base&&) noexcept requires(TriviallyCopyable<T>) = default;

  constexpr copy_optional_base(const copy_optional_base& other) noexcept(std::is_nothrow_copy_constructible_v<T>) requires(!TriviallyCopyable<T>) {
    this->active = other.active;
    if (other.active) {
      new (&(this->value)) T(other.value);
    }
  }

  constexpr copy_optional_base(copy_optional_base&& other) noexcept(std::is_nothrow_move_constructible_v<T>) requires(!TriviallyCopyable<T>) {
    this->active = other.active;
    if (other.active) {
      new (&(this->value)) T(std::move(other.value));
    }
  }

  constexpr copy_optional_base& operator=(const copy_optional_base& other) noexcept(std::is_nothrow_copy_assignable_v<T>) requires(!TriviallyCopyable<T>) {
    if (this != &other) {
      if (this->active) {
        if (other.active) {
          this->value = other.value;
        } else {
          this->reset();
          this->dummy = dummy_t();
        }
      } else if (other.active) {
        new (&(this->value)) T(other.value);
      }
      this->active = other.active;
    }
    return *this;
  }

  constexpr copy_optional_base& operator=(copy_optional_base&& other) noexcept(std::is_nothrow_move_assignable_v<T>) requires(!TriviallyCopyable<T>) {
    if (this != &other) {
      if (this->active) {
        if (other.active) {
          this->value = std::move(other.value);
        } else {
          this->reset();
          this->dummy = dummy_t();
        }
      } else if (other.active) {
        new (&(this->value)) T(std::move(other.value));
      }
      this->active = other.active;
    }
    return *this;
  }
};

} // namespace detail

template <typename T>
class optional : detail::copy_optional_base<T> {
  using base = detail::copy_optional_base<T>;
  using base::base;

public:
  constexpr optional() noexcept = default;
  constexpr optional(const optional&) noexcept(std::is_nothrow_copy_constructible_v<T>) requires(detail::CopyConstructible<T>) = default;
  constexpr optional(optional&&) noexcept(std::is_nothrow_move_constructible_v<T>) requires(detail::MoveConstructible<T>) = default;
  constexpr optional& operator=(const optional&) noexcept(std::is_nothrow_copy_assignable_v<T>) requires(detail::CopyAssignable<T>) = default;
  constexpr optional& operator=(optional&&) noexcept(std::is_nothrow_move_assignable_v<T>) requires(detail::MoveAssignable<T>) = default;
  constexpr optional(nullopt_t) noexcept : optional() {};

  constexpr optional& operator=(nullopt_t) noexcept {
    reset();
    return *this;
  }

  template <typename... Args> requires detail::Constructible<T, Args...>
  constexpr void emplace(Args... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    reset();
    new (&(this->value)) T(std::forward<Args>(args)...);
    this->active = true;
  }

  bool has_value() const noexcept {
    return this->active;
  }

  constexpr T& operator*() noexcept {
    return this->value;
  }

  constexpr T const& operator*() const noexcept {
    return this->value;
  }

  constexpr T const* operator->() const noexcept {
    return &this->value;
  }

  constexpr T* operator->() noexcept {
    return &this->value;
  }

  constexpr explicit operator bool() const noexcept {
    return this->active;
  }

  constexpr void reset() noexcept {
    base::reset();
  }

  friend constexpr bool operator==(const optional<T>& lhs, const optional<T>& rhs) {
    if (bool(lhs) != bool(rhs)) {
      return false;
    }
    if (!lhs) {
      return true;
    }
    return lhs.value == rhs.value;
  }

  friend constexpr bool operator!=(const optional<T>& lhs, const optional<T>& rhs) {
    return !(lhs == rhs);
  }

  friend constexpr bool operator<(const optional<T>& lhs, const optional<T>& rhs) {
    if (!rhs) {
      return false;
    }
    if (!lhs) {
      return true;
    }
    return lhs.value < rhs.value;
  }

  friend constexpr bool operator<=(const optional<T>& lhs, const optional<T>& rhs) {
    return lhs < rhs || lhs == rhs;
  }

  friend constexpr bool operator>(const optional<T>& lhs, const optional<T>& rhs) {
    return !(lhs <= rhs);
  }

  friend constexpr bool operator>=(const optional<T>& lhs, const optional<T>& rhs) {
    return !(lhs < rhs);
  }
};
