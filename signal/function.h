#pragma once

#include <exception>

struct bad_function_call : std::exception {
  char const* what() const noexcept override {
    return "bad function call";
  }
};

static constexpr size_t SIZE_OF_STORAGE = sizeof(void*);
static constexpr size_t ALIGN_OF_STORAGE = alignof(void*);

using storage_t = std::aligned_storage_t<SIZE_OF_STORAGE, ALIGN_OF_STORAGE>;

template <typename F>
static constexpr bool is_small = sizeof(F) <= SIZE_OF_STORAGE &&
                                 ALIGN_OF_STORAGE % alignof(F) == 0 &&
                                 std::is_nothrow_move_constructible_v<F>;

template <typename R, typename... Args>
struct concept_t {
  R (*invoke)(storage_t const *, Args...);
  void (*destroy)(storage_t*);
  void (*copy)(storage_t*, storage_t const*);
  void (*move)(storage_t*, storage_t*);
};

template <typename R, typename... Args>
const concept_t<R, Args...>* empty_operations() noexcept {

  constexpr static concept_t<R, Args...> value = {
      [](storage_t const *, Args...) -> R { throw bad_function_call(); },
      [](storage_t*) -> void {},
      [](storage_t*, storage_t const*) -> void {},
      [](storage_t*, storage_t*) -> void {}};

  return &value;
}

template <typename F, typename R, typename... Args>
const concept_t<R, Args...>* operations() noexcept {

  static constexpr F const * (*get_small)(storage_t const*) =
  [](storage_t const *ptr) -> F const * { return reinterpret_cast<F const *>(ptr); };

  static constexpr F* const (*get_big)(storage_t const*) =
  [](storage_t const *ptr) -> F* const { return *reinterpret_cast<F * const*>(ptr); };

  static constexpr void (*set_big)(storage_t *, F*) =
  [](storage_t *ptr, F* value) -> void { *reinterpret_cast<F**>(ptr) = value; };


  constexpr static concept_t<R, Args...> value = {
      [](storage_t const *ptr, Args... args) -> R {
        if constexpr (is_small<F>) {
          return (*get_small(ptr))(std::forward<Args>(args)...);
        } else {
          return (*get_big(ptr))(std::forward<Args>(args)...);
        }
      },
      [](storage_t* ptr) -> void {
        if constexpr (is_small<F>) {
          get_small(ptr)->~F();
        } else {
          delete get_big(ptr);
        }
      },
      [](storage_t* dst, storage_t const* src) -> void {
        if constexpr (is_small<F>) {
          new (dst) F(*get_small(src));
        } else {
          set_big(dst, new F(*get_big(src)));
        }
      },
      [](storage_t* dst, storage_t* src) -> void {
        if constexpr (is_small<F>) {
          new (dst) F(std::move(*get_small(src)));
        } else {
          set_big(dst, get_big(src));
        }
      }
  };

  return &value;
};

template <typename F>
struct function;

template <typename R, typename... Args>
struct function<R(Args...)> {
  function() noexcept : ops(empty_operations<R, Args...>()) {}

  function(function const& other) : ops(other.ops) {
    ops->copy(&stg, &other.stg);
  }

  function(function&& other) noexcept : ops(other.ops) {
    ops->move(&stg, &other.stg);
    other.ops = empty_operations<R, Args...>();
  }

  template <typename F>
  function(F f) : ops(operations<F, R, Args...>()) {
    if constexpr (is_small<F>) {
      new (&stg) F(std::move(f));
    } else {
      set_big(&stg, new F(std::move(f)));
    }
  }

  function& operator=(function const& rhs) {
    if (this != &rhs) {
      swap(function(rhs));
    }
    return *this;
  }

  function& operator=(function&& rhs) noexcept {
    if (this != &rhs) {
      swap(function(std::move(rhs)));
    }
    return *this;
  }

  ~function() {
    ops->destroy(&stg);
  }

  explicit operator bool() const noexcept {
    return ops != empty_operations<R, Args...>();
  }

  R operator()(Args... args) const {
    return ops->invoke(&stg, std::forward<Args>(args)...);
  }

  template <typename F>
  F* target() noexcept {
    return ops == operations<F, R, Args...>() ? get_obj<F>() : nullptr;
  }

  template <typename F>
  F const* target() const noexcept {
    return ops == operations<F, R, Args...>() ? get_obj<F>() : nullptr;
  }

private:
  storage_t stg;
  concept_t<R, Args...> const* ops;

  void swap(function&& other) noexcept {
    std::swap(stg, other.stg);
    std::swap(ops, other.ops);
  }

  template <typename F>
  F const * get_obj() const noexcept {
    if constexpr (is_small<F>) {
      return reinterpret_cast<F const *>(&stg);
    } else {
      return *reinterpret_cast<F* const *>(&stg);
    }
  }

  template <typename F>
  F * get_obj() noexcept {
    if constexpr (is_small<F>) {
      return reinterpret_cast<F*>(&stg);
    } else {
      return *reinterpret_cast<F**>(&stg);
    }
  }

  template <typename F>
  F* set_big(storage_t *ptr, F* value) {
    return *reinterpret_cast<F**>(ptr) = value;
  }
};
