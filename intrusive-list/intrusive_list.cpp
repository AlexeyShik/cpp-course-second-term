#include "intrusive_list.h"

intrusive::list_element_base::list_element_base() noexcept: next(this), prev(this) {}

intrusive::list_element_base::~list_element_base() noexcept {
    unlink();
}

void intrusive::list_element_base::unlink() noexcept {
    next->prev = prev;
    prev->next = next;

    next = this;
    prev = this;
}

void
intrusive::list_element_base::splice(intrusive::list_element_base &first, intrusive::list_element_base &last) noexcept {
    swap3(prev->next, first.prev->next, last.prev->next);
    swap3(prev, last.prev, first.prev);
}

void
intrusive::list_element_base::swap3(intrusive::list_element_base *&a, intrusive::list_element_base *&b,
                                         intrusive::list_element_base *&c) {
    intrusive::list_element_base *copy = a;
    a = b;
    b = c;
    c = copy;
}