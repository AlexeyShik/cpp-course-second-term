#pragma once

#include "control_block.h"

namespace shared_ptr_details {

template <typename T, typename D = std::default_delete<T>>
struct ptr_block final : public shared_ptr_details::control_block, D {

  explicit ptr_block(T* ptr_, D deleter_) noexcept
      : ptr(ptr_), D(std::move(deleter_)) {
    this->inc_strong();
  }

  void do_delete() override {
    D::operator()(ptr);
  }

private:
  T* ptr{nullptr};
};

}

