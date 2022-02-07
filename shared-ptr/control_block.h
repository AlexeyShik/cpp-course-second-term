#pragma once

template <typename T>
class shared_ptr;

namespace shared_ptr_details { // hiding details of realization

struct control_block {

  void inc_weak() noexcept;
  void inc_strong() noexcept;
  void dec_weak();
  void dec_strong();
  int get_strong_cnt() const noexcept;

  virtual void do_delete() = 0;
  virtual ~control_block() = default;

private:
  int strong_cnt{0};
  int weak_cnt{0};
};

} // namespace details

