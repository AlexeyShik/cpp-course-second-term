#include "intrusive_tree.h"

intrusive::tree_element_base::~tree_element_base() noexcept {
  unlink();
}

intrusive::tree_element_base&
intrusive::tree_element_base::operator=(tree_element_base&& other) noexcept {
  if (this != &other) {
    left = other.left;
    right = other.right;
    parent = other.parent;
    other.unlink();
  }
  return *this;
}

void intrusive::tree_element_base::unlink() noexcept {
  right = nullptr;
  left = nullptr;
  parent = nullptr;
}

intrusive::tree_element_base::tree_element_base(
    tree_element_base&& other) noexcept
    : left(other.left), right(other.right), parent(other.parent) {
  if (this != &other) {
    other.unlink();
  }
}
