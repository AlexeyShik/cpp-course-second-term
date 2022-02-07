#pragma once

#include <cstddef>
#include <memory>
#include <utility>

#include "control_block.h"
#include "obj_block.h"
#include "ptr_block.h"
#include "weak_ptr.h"

using namespace shared_ptr_details;

template <typename T>
class weak_ptr;

template <typename T>
class shared_ptr {
  friend weak_ptr<T>;

  template <typename OT>
  friend class shared_ptr;

public:
  shared_ptr() noexcept = default;
  explicit shared_ptr(std::nullptr_t) noexcept;

  template <typename RT, typename D = std::default_delete<RT>,
            typename = std::enable_if_t<std::is_convertible_v<RT*, T*>>>
  explicit shared_ptr(RT* ptr_, D&& deleter_ = std::default_delete<RT>()) {
    try {
      block = new ptr_block<RT, D>(ptr_, std::forward<D>(deleter_));
      ptr = ptr_;
    } catch (...) {
      deleter_(ptr_);
      throw;
    }
  }
  template <class OT>
  shared_ptr(const shared_ptr<OT>&, T*) noexcept;
  shared_ptr(const shared_ptr<T>&) noexcept;

  template <typename OT, typename = std::enable_if_t<std::is_convertible_v<OT*, T*>>>
  shared_ptr(const shared_ptr<OT>& other) noexcept
      : block(other.block), ptr(other.ptr) {
    if (block) {
      block->inc_strong();
    }
  }
  shared_ptr(shared_ptr<T>&&) noexcept;
  ~shared_ptr() noexcept;

  shared_ptr& operator=(const shared_ptr<T>&) noexcept;
  shared_ptr& operator=(shared_ptr<T>&&) noexcept;

  T* get() const noexcept;
  operator bool() const noexcept;
  T& operator*() const noexcept;
  T* operator->() const noexcept;

  bool operator==(const shared_ptr<T>&) const noexcept;
  bool operator!=(const shared_ptr<T>&) const noexcept;

  std::size_t use_count() const noexcept;
  void reset() noexcept;

  template <typename RT, typename D = std::default_delete<RT>,
            typename = std::enable_if_t<std::is_base_of_v<T, RT>>>
  void reset(RT* new_ptr, D &&deleter_ = std::default_delete<RT>()) noexcept {
    swap(shared_ptr<T>(new_ptr, std::forward<D>(deleter_)));
  }

  template <typename TT, typename... Args>
  friend shared_ptr<TT> make_shared(Args&&... args);

private:
  control_block* block{nullptr};
  T* ptr{nullptr};

  explicit shared_ptr(control_block*) noexcept;
  explicit shared_ptr(control_block*, T*) noexcept;
  void swap(shared_ptr<T>&& other) noexcept;
};

template <typename T>
shared_ptr<T>::shared_ptr(std::nullptr_t) noexcept {}

template<typename T>
shared_ptr<T>::shared_ptr(const shared_ptr<T>& other) noexcept
    : block(other.block), ptr(other.ptr) {
  if (block) {
    block->inc_strong();
  }
}

template <typename T>
shared_ptr<T>::shared_ptr(control_block* block_) noexcept : block(block_) {
  if (block != nullptr) {
    block->inc_strong();
  }
}

template <typename T>
shared_ptr<T>::shared_ptr(control_block* block_, T* ptr_) noexcept
    : block(block_), ptr(ptr_) {
  if (block != nullptr) {
    block->inc_strong();
  }
}

template <typename T>
shared_ptr<T>::shared_ptr(shared_ptr<T>&& other) noexcept {
  swap(std::move(other));
}

template <class T>
template <class OT>
shared_ptr<T>::shared_ptr(const shared_ptr<OT>& other, T* ptr_) noexcept
    : shared_ptr(other.block, ptr_) {}

template <typename T>
shared_ptr<T>::~shared_ptr() noexcept {
  if (block) {
    block->dec_strong();
  }
}

template <typename T>
void shared_ptr<T>::swap(shared_ptr<T>&& other) noexcept {
  std::swap(block, other.block);
  std::swap(ptr, other.ptr);
}

template <typename T>
shared_ptr<T>& shared_ptr<T>::operator=(const shared_ptr& other) noexcept {
  if (*this != other) {
    swap(shared_ptr<T>(other));
  }
  return *this;
}

template <typename T>
shared_ptr<T>& shared_ptr<T>::operator=(shared_ptr&& other) noexcept {
  if (*this != other) {
    swap(shared_ptr<T>(std::move(other)));
  }
  return *this;
}

template <typename T>
T* shared_ptr<T>::get() const noexcept {
  return block == nullptr ? nullptr : ptr;
}

template <typename T>
shared_ptr<T>::operator bool() const noexcept {
  return get() != nullptr;
}

template <typename T>
T& shared_ptr<T>::operator*() const noexcept {
  return *get();
}

template <typename T>
T* shared_ptr<T>::operator->() const noexcept {
  return get();
}

template <typename T>
std::size_t shared_ptr<T>::use_count() const noexcept {
  return block == nullptr ? 0 : block->get_strong_cnt();
}

template <typename T>
void shared_ptr<T>::reset() noexcept {
  swap(shared_ptr<T>());
}

template <typename T>
bool shared_ptr<T>::operator==(const shared_ptr<T>& other) const noexcept {
  return get() == other.get();
}

template <typename T>
bool shared_ptr<T>::operator!=(const shared_ptr<T>& other) const noexcept {
  return !operator==(other);
}

template <typename T>
bool operator==(const shared_ptr<T>& p1, std::nullptr_t) noexcept {
  return p1.get() == nullptr;
}

template <typename T>
bool operator!=(const shared_ptr<T>& p1, std::nullptr_t p2) noexcept {
  return !(p1 == p2);
}

template <typename T>
bool operator==(std::nullptr_t, const shared_ptr<T>& p2) noexcept {
  return p2.get() == nullptr;
}

template <typename T>
bool operator!=(std::nullptr_t p1, const shared_ptr<T>& p2) noexcept {
  return !(p1 == p2);
}

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
  auto* block = new obj_block<T>(std::forward<Args>(args)...);
  return shared_ptr<T>(block, reinterpret_cast<T*>(&block->data));
}

