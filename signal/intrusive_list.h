#pragma once

#include <iterator>

namespace intrusive {
/*
Тег по-умолчанию чтобы пользователям не нужно было
придумывать теги, если они используют лишь одну базу
list_element.
*/
struct default_tag;

struct list_element_base {
  // list work

  list_element_base() noexcept;

  ~list_element_base() noexcept;

  list_element_base(list_element_base const &) = delete;

  list_element_base &operator=(list_element_base const &) = delete;

  list_element_base &operator=(list_element_base &&other);

  /* Отвязывает элемент из списка в котором он находится. */
  void unlink() noexcept;

  void splice(list_element_base &first, list_element_base &last) noexcept;

  static void swap3(list_element_base* &a, list_element_base* &b, list_element_base* &c);

  list_element_base *next;
  list_element_base *prev;
};

template<typename Tag = default_tag>
struct list_element : list_element_base {

  template<typename ListTag>
  friend list_element_base &to_base(list_element<ListTag> &listElement) noexcept;

  template<typename ListTag>
  friend list_element<ListTag> &from_base(list_element_base &listElementBase) noexcept;

  list_element<Tag> &operator=(list_element<Tag> &&other) noexcept {
    return static_cast<list_element<Tag> &>(to_base(*this) = std::move(to_base(other)));
  }
};

template<typename Tag = default_tag>
list_element_base &to_base(list_element<Tag> &listElement) noexcept {
  return static_cast<list_element_base &>(listElement);
}

template<typename Tag = default_tag>
list_element<Tag> &from_base(list_element_base &listElementBase) noexcept {
  return static_cast<list_element<Tag> &>(listElementBase);
}

template<typename T, typename Tag = default_tag>
struct list {
  static_assert(std::is_base_of_v<list_element<Tag>, T>,
                "You should derive from list_element");

public:
  list() = default;

  list(list const &) = delete;

  list(list &&other) noexcept {
    swap_and_clear(other);
  }

  ~list() noexcept {
    clear();
  };

  list &operator=(list const &) = delete;

  list &operator=(list &&other) noexcept {
    if (this != &other) {
      swap_and_clear(other);
    }
    return *this;
  }

  void clear() noexcept {
    while (!empty()) {
      pop_back();
    }
  }

  /*
  Поскольку вставка изменяет данные в list_element
  мы принимаем неконстантный T&.
  */
  void push_back(T &value) noexcept {
    insert(end(), value);
  }

  void pop_back() noexcept {
    fake_node.prev->unlink();
  }

  T &back() noexcept {
    return from_list_element_base(*fake_node.prev);
  }

  T const &back() const noexcept {
    return from_list_element_base(*fake_node.prev);
  }

  void push_front(T &value) noexcept {
    insert(begin(), value);
  }

  void pop_front() noexcept {
    fake_node.next->unlink();
  }

  T &front() noexcept {
    return from_list_element_base(*fake_node.next);
  };

  T const &front() const noexcept {
    return from_list_element_base(*fake_node.next);
  };

  bool empty() const noexcept {
    return fake_node.prev == &fake_node;
  };

  template<typename IT = T>
  struct list_iterator {

    friend list;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = std::remove_const_t<T>;
    using reference = value_type &;
    using pointer = value_type *;
    using difference_type = std::ptrdiff_t;

    list_iterator() = default;

    template<typename OtherIT>
    list_iterator(
        list_iterator<OtherIT> other,
        std::enable_if<
            std::is_same_v<std::remove_const<IT>, OtherIT> && std::is_const<IT>::value,
            OtherIT> * = nullptr) noexcept : data(other.data) {}

    list_iterator &operator++() noexcept {
      data = data->next;
      return *this;
    }

    list_iterator operator++(int) noexcept {
      list_iterator copy = *this;
      data = data->next;
      return copy;
    }

    list_iterator &operator--() noexcept {
      data = data->prev;
      return *this;
    }

    list_iterator operator--(int) noexcept {
      list_iterator copy = *this;
      data = data->prev;
      return copy;
    }

    IT *operator->() const noexcept {
      return &operator*();
    }

    IT &operator*() const noexcept {
      return static_cast<IT &>(from_base<Tag>(*data));
    }

    bool operator==(list_iterator const &other) const noexcept {
      return data == other.data;
    }

    bool operator!=(list_iterator const &other) const noexcept {
      return !(*this == other);
    }

  private:
    explicit list_iterator(T &value) : list_iterator(static_cast<list_element_base*>(&value)) {}

    explicit list_iterator(list_element_base *data) noexcept : data(data) {}

    explicit list_iterator(const list_element_base *data) noexcept : data(const_cast<list_element_base *>(data)) {}

    list_element_base *data;
  };

  using iterator = list_iterator<T>;
  using const_iterator = list_iterator<const T>;

  iterator begin() noexcept {
    return iterator(fake_node.next);
  }

  const_iterator begin() const noexcept {
    return const_iterator(fake_node.next);
  }

  iterator end() noexcept {
    return iterator(&fake_node);
  }

  const_iterator end() const noexcept {
    return const_iterator(&fake_node);
  }

  iterator insert(const_iterator pos, T &value) noexcept {
    list_element_base &node = to_list_element_base(value);

    if (pos.data == &node) {
      return pos;
    }

    list_element_base *left = pos.data->prev;
    list_element_base *right = pos.data;
    node.unlink();

    left->next = &node;
    right->prev = &node;
    node.prev = left;
    node.next = right;
    return iterator(&node);
  }

  iterator erase(const_iterator pos) noexcept {
    list_element_base *next = pos.data->next;
    pos.data->unlink();
    return iterator(next);
  }

  void splice(const_iterator pos, list &list, const_iterator first, const_iterator last) noexcept {
    if (first == last) {
      return;
    }

    pos.data->splice(*first.data, *last.data);
  }

  static iterator to_list_iterator(T &value) {
    return iterator(value);
  }

private:

  void swap_and_clear(list &other) noexcept {
    swap(fake_node, other.fake_node);
    other.clear();
  }

  static void swap(list_element_base &first, list_element_base &second) noexcept {
    std::swap(first.prev, second.prev);
    std::swap(first.prev->next, second.prev->next);
    std::swap(first.next, second.next);
    std::swap(first.next->prev, second.next->prev);
  }

  static list_element<Tag> &to_list_element(T &value) noexcept {
    return static_cast<list_element<Tag> &>(value);
  }

  static list_element_base &to_list_element_base(T &value) noexcept {
    return to_base(to_list_element(value));
  }

  static T &from_list_element(list_element<Tag> &listElement) noexcept {
    return static_cast<T &>(listElement);
  }

  static T &from_list_element_base(list_element_base &node) noexcept {
    return from_list_element(from_base<Tag>(node));
  }

  list_element_base fake_node;
};
} // namespace intrusive
