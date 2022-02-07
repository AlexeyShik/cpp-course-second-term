#pragma once

#include "control_block.h"

using namespace shared_ptr_details;

template <typename T>
class weak_ptr {
  friend shared_ptr<T>;

public:
  weak_ptr() noexcept = default;

  ~weak_ptr() noexcept;
  weak_ptr(const shared_ptr<T>&) noexcept;
  weak_ptr(const weak_ptr<T>&) noexcept;
  weak_ptr(weak_ptr<T>&&) noexcept;
  weak_ptr& operator=(const shared_ptr<T>&) noexcept;
  weak_ptr& operator=(const weak_ptr<T>&) noexcept;
  weak_ptr& operator=(weak_ptr<T>&&) noexcept;

  shared_ptr<T> lock() const noexcept;

private:
  control_block* block{nullptr};
  T* ptr{nullptr};

  weak_ptr(control_block*, T*) noexcept;
  void swap(weak_ptr<T>&) noexcept;
  void swap(weak_ptr<T>&&) noexcept;
};

template <typename T>
weak_ptr<T>::weak_ptr(control_block* block_, T* ptr_) noexcept
    : block(block_), ptr(ptr_) {
  if (block) {
    block->inc_weak();
  }
}

template <typename T>
weak_ptr<T>::weak_ptr(const shared_ptr<T>& other) noexcept
    : weak_ptr(other.block, other.ptr) {}

template <typename T>
weak_ptr<T>::weak_ptr(const weak_ptr<T>& other) noexcept
    : weak_ptr(other.block, other.ptr) {}

template <typename T>
weak_ptr<T>::weak_ptr(weak_ptr<T>&& other) noexcept {
  swap(other);
}

template <typename T>
weak_ptr<T>::~weak_ptr() noexcept {
  if (block) {
    block->dec_weak();
  }
}

template <typename T>
weak_ptr<T>& weak_ptr<T>::operator=(const shared_ptr<T>& other) noexcept {
  swap(weak_ptr<T>(other));
  return *this;
}

template <typename T>
weak_ptr<T>& weak_ptr<T>::operator=(const weak_ptr<T>& other) noexcept {
  if (this != &other) {
    swap(weak_ptr<T>(other));
  }
  return *this;
}

template <typename T>
weak_ptr<T>& weak_ptr<T>::operator=(weak_ptr<T>&& other) noexcept {
  swap(weak_ptr<T>(std::move(other)));
  return *this;
}

template <typename T>
shared_ptr<T> weak_ptr<T>::lock() const noexcept {
  if (block == nullptr || block->get_strong_cnt() == 0) {
    return shared_ptr<T>();
  }
  return shared_ptr<T>(block, ptr);
}

template <typename T>
void weak_ptr<T>::swap(weak_ptr<T>& other) noexcept {
  std::swap(block, other.block);
  std::swap(ptr, other.ptr);
}
template <typename T>
void weak_ptr<T>::swap(weak_ptr<T>&& other) noexcept {
  swap(other);
}

