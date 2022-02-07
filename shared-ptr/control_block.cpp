#include "control_block.h"

void shared_ptr_details::control_block::inc_weak() noexcept {
  ++weak_cnt;
}

void shared_ptr_details::control_block::inc_strong() noexcept {
  ++strong_cnt;
  inc_weak();
}

void shared_ptr_details::control_block::dec_weak() {
  --weak_cnt;
  if (weak_cnt == 0) {
    delete this;
  }
}

void shared_ptr_details::control_block::dec_strong() {
  --strong_cnt;
  if (strong_cnt == 0) {
    do_delete();
  }
  dec_weak();
}

int shared_ptr_details::control_block::get_strong_cnt() const noexcept {
  return strong_cnt;
}

