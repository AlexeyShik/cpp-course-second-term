#pragma once

#include "control_block.h"

namespace shared_ptr_details {

template <typename T>
struct obj_block final : public shared_ptr_details::control_block {

  std::aligned_storage_t<sizeof(T), alignof(T)> data;

  template <typename... Args>
  explicit obj_block(Args... args) noexcept {
    new (&data) T(std::forward<Args>(args)...);
  }

  void do_delete() override {
    reinterpret_cast<T*>(&data)->~T();
  }

};

}

