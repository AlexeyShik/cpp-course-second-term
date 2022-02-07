#pragma once

#include "intrusive_tree.h"

struct left_tag {};
struct right_tag {};

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap
    : private intrusive::intrusive_tree<Left, CompareLeft, left_tag>,
      private intrusive::intrusive_tree<Right, CompareRight, right_tag> {

  template <typename T>
  struct iterator;

private:

  using left_t = Left;
  using right_t = Right;

  using left_comp_t = CompareLeft;
  using right_comp_t = CompareRight;

  using left_tree_t = intrusive::intrusive_tree<left_t, left_comp_t, left_tag>;
  using right_tree_t = intrusive::intrusive_tree<right_t, right_comp_t, right_tag>;

  using left_node_t = typename left_tree_t::node_t;
  using right_node_t = typename right_tree_t ::node_t;

  using left_iterator_t = typename left_tree_t::iterator;
  using right_iterator_t = typename right_tree_t::iterator;

  using left_iterator = iterator<left_tree_t>;
  using right_iterator = iterator<right_tree_t>;

  struct node_t : left_node_t, right_node_t {

    template <typename L = left_t, typename R = right_t>
    node_t(L&& left, R&& right) noexcept
        : left_node_t(std::forward<L>(left)),
          right_node_t(std::forward<R>(right)) {}
  };

  static left_node_t* to_left_node(node_t* node) {
    return static_cast<left_node_t*>(node);
  }

  static right_node_t* to_right_node(node_t* node) {
    return static_cast<right_node_t*>(node);
  }

  static node_t* from_left_node(left_node_t* left) {
    return static_cast<node_t*>(left);
  }

  static node_t* from_right_node(right_node_t* right) {
    return static_cast<node_t*>(right);
  }

  left_tree_t& get_left_tree() const noexcept {
    return const_cast<left_tree_t&>(static_cast<const left_tree_t&>(*this));
  }

  right_tree_t& get_right_tree() const noexcept {
    return const_cast<right_tree_t&>(static_cast<const right_tree_t&>(*this));
  }

  size_t sz{};

public:
  template <typename V, typename Comp, typename Tag>
  struct iterator<intrusive::intrusive_tree<V, Comp, Tag>> {

    template <typename OtherTree>
    friend struct iterator;

    friend bimap;

    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = std::remove_const_t<V>;
    using reference = value_type&;
    using pointer = value_type*;
    using difference_type = std::ptrdiff_t;

    // Элемент на который сейчас ссылается итератор.
    // Разыменование итератора end_left() неопределено.
    // Разыменование невалидного итератора неопределено.
    V const& operator*() const noexcept {
      return it.operator*();
    }

    V const* operator->() const noexcept {
      return it.operator->();
    }

    // Переход к следующему по величине left'у.
    // Инкремент итератора end_left() неопределен.
    // Инкремент невалидного итератора неопределен.
    iterator& operator++() noexcept {
      ++it;
      return *this;
    }

    iterator operator++(int) noexcept {
      auto copy = *this;
      it++;
      return copy;
    }

    // Переход к предыдущему по величине left'у.
    // Декремент итератора begin_left() неопределен.
    // Декремент невалидного итератора неопределен.
    iterator& operator--() noexcept {
      --it;
      return *this;
    }

    iterator operator--(int) noexcept {
      auto copy = *this;
      it--;
      return copy;
    }

    friend bool operator==(const iterator& i1, const iterator& i2) {
      return i1.it == i2.it;
    }

    friend bool operator!=(const iterator& i1, const iterator& i2) {
      return !operator==(i1, i2);
    }

    // left_iterator ссылается на левый элемент некоторой пары.
    // Эта функция возвращает итератор на правый элемент той же пары.
    // end_left().flip() возращает end_right().
    // end_right().flip() возвращает end_left().
    // flip() невалидного итератора неопределен.
    decltype(auto) flip() const noexcept {

      using opposite_iterator = std::conditional_t<std::is_same_v<Tag, left_tag>,
          right_iterator, left_iterator>;
      using opposite_iterator_t = std::conditional_t<std::is_same_v<Tag, left_tag>,
          right_iterator_t, left_iterator_t>;
      using opposite_tag = std::conditional_t<std::is_same_v<Tag, left_tag>,
          right_tag, left_tag>;
      using opposite_tree_t = std::conditional_t<std::is_same_v<Tag, left_tag>,
          right_tree_t, left_tree_t>;
      using opposite_node_t = std::conditional_t<std::is_same_v<Tag, left_tag>,
          right_node_t, left_node_t>;
      using tree_t = std::conditional_t<std::is_same_v<Tag, left_tag>,
          left_tree_t, right_tree_t>;

      if (it.get_base()->parent == nullptr) {
        // == end_left()
        return opposite_iterator(
            opposite_iterator_t(
                static_cast<intrusive::tree_element<opposite_tag>*>(
                    static_cast<opposite_tree_t*>(
                        static_cast<bimap*>(
                            static_cast<tree_t*>(
                                it.get_elem()))))));
      }
      return opposite_iterator(
          opposite_iterator_t(
              static_cast<opposite_node_t*>(
                  static_cast<node_t*>(it.get_node()))));
    }

  private:
    using tree_iterator_t =
        typename intrusive::intrusive_tree<V, Comp, Tag>::iterator;

    tree_iterator_t it{};

    iterator(tree_iterator_t it) noexcept : it(it) {}
  };

  // Создает bimap не содержащий ни одной пары.
  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight()) noexcept
      : left_tree_t(std::move(compare_left)),
        right_tree_t(std::move(compare_right)) {}

  // Конструкторы от других и присваивания
  bimap(bimap const& other)
      : bimap(other.get_left_tree().get_comparator(),
              other.get_right_tree().get_comparator()) {
    for (auto it = other.begin_left(); it != other.end_left(); ++it) {
      insert(*it, *it.flip());
    }
  }

  bimap& operator=(bimap const& other) {
    if (this != &other) {
      bimap tmp(other);
      swap(tmp);
    }
    return *this;
  }

  // Деструктор. Вызывается при удалении объектов bimap.
  // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
  // (включая итераторы ссылающиеся на элементы следующие за последними).
  ~bimap() {
    while (!empty()) {
      erase_left(begin_left());
    }
  }

  // Вставка пары (left, right), возвращает итератор на left.
  // Если такой left или такой right уже присутствуют в bimap, вставка не
  // производится и возвращается end_left().
  template <typename L = left_t, typename R = right_t>
  left_iterator insert(L&& left, R&& right) {
    if (find_left(left) != end_left() || find_right(right) != end_right()) {
      return end_left();
    }
    auto* node = new node_t(std::forward<L>(left), std::forward<R>(right));
    ++sz;
    get_right_tree().insert(to_right_node(node));
    return get_left_tree().insert(to_left_node(node));
  }

  left_iterator insert(const left_t& left, const right_t& right) {
    if (find_left(left) != end_left() || find_right(right) != end_right()) {
      return end_left();
    }
    auto* node = new node_t(left, right);
    ++sz;
    get_right_tree().insert(to_right_node(node));
    return get_left_tree().insert(to_left_node(node));
  }

  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator const& it) {
    if (it == end_left()) {
      return end_left();
    }
    --sz;
    left_iterator res = left_iterator(get_left_tree().erase(it.it));
    get_right_tree().erase(it.flip().it);
    delete from_left_node(it.it.get_node());
    return res;
  }

  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
  bool erase_left(left_t const& left) {
    auto it = get_left_tree().find(left);
    if (it == end_left()) {
      return false;
    }
    erase_left(it);
    return true;
  }

  right_iterator erase_right(right_iterator const& it) {
    if (it == end_right()) {
      return end_right();
    }
    --sz;
    right_iterator res = right_iterator(get_right_tree().erase(it.it));
    get_left_tree().erase(it.flip().it);
    delete from_right_node(it.it.get_node());
    return res;
  }

  bool erase_right(right_t const& right) {
    auto it = get_right_tree().find(right);
    if (it == end_right()) {
      return false;
    }
    erase_right(it);
    return true;
  }

  // erase от ренжа, удаляет [first, last), возвращает итератор на последний
  // элемент за удаленной последовательностью
  left_iterator erase_left(left_iterator first, left_iterator last) {
    while (first != last) {
      first = erase_left(first);
    }
    return last;
  }

  right_iterator erase_right(right_iterator first, right_iterator last) {
    while (first != last) {
      first = erase_right(first);
    }
    return last;
  }

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  left_iterator find_left(left_t const& left) const {
    return get_left_tree().find(left);
  }

  right_iterator find_right(right_t const& right) const {
    return get_right_tree().find(right);
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  right_t const& at_left(left_t const& key) const {
    auto it = find_left(key);
    if (it == end_left()) {
      throw std::out_of_range("Expected existing key");
    }
    return *it.flip();
  }

  left_t const& at_right(right_t const& key) const {
    auto it = find_right(key);
    if (it == end_right()) {
      throw std::out_of_range("Expected existing key");
    }
    return *it.flip();
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)
  template <typename R = right_t,
            typename = std::enable_if_t<std::is_default_constructible_v<R>>>
  right_t const& at_left_or_default(left_t const& key) {
    auto left_it = find_left(key);
    if (left_it != end_left()) {
      return *left_it.flip();
    }
    auto right_key = right_t();
    auto right_it = find_right(right_key);
    if (right_it != end_right()) {
      erase_right(right_it);
    }
    return *insert(key, std::move(right_key)).flip();
  }

  template <typename L = left_t,
            typename = std::enable_if_t<std::is_default_constructible_v<L>>>
  left_t const& at_right_or_default(right_t const& key) {
    auto right_it = find_right(key);
    if (right_it != end_right()) {
      return *right_it.flip();
    }
    auto left_key = left_t();
    auto left_it = find_left(left_key);
    if (left_it != end_left()) {
      erase_left(left_it);
    }
    return *insert(std::move(left_key), key);
  }

  // lower и upper bound'ы по каждой стороне
  // Возвращают итераторы на соответствующие элементы
  // Смотри std::lower_bound, std::upper_bound.
  left_iterator lower_bound_left(const left_t& left) const noexcept {
    return get_left_tree().lower_bound(left);
  }

  left_iterator upper_bound_left(const left_t& left) const noexcept {
    return get_left_tree().upper_bound(left);
  }

  right_iterator lower_bound_right(const right_t& right) const noexcept {
    return get_right_tree().lower_bound(right);
  }

  right_iterator upper_bound_right(const right_t& right) const noexcept {
    return get_right_tree().upper_bound(right);
  }

  // Возващает итератор на минимальный по порядку left.
  left_iterator begin_left() const noexcept {
    return left_iterator(get_left_tree().begin());
  }

  // Возващает итератор на следующий за последним по порядку left.
  left_iterator end_left() const noexcept {
    return left_iterator(get_left_tree().end());
  }

  // Возващает итератор на минимальный по порядку right.
  right_iterator begin_right() const noexcept {
    return right_iterator(get_right_tree().begin());
  }

  // Возващает итератор на следующий за последним по порядку right.
  right_iterator end_right() const noexcept {
    return right_iterator(get_right_tree().end());
  }

  // Проверка на пустоту
  bool empty() const noexcept {
    return sz == 0;
  }

  // Возвращает размер бимапы (кол-во пар)
  std::size_t size() const noexcept {
    return sz;
  }

  // операторы сравнения
  friend bool operator==(bimap const& a, bimap const& b) {
    auto a_it = a.begin_left();
    auto b_it = b.begin_left();
    for (; a_it != a.end_left() && b_it != b.end_left(); ++a_it, ++b_it) {
      if (*a_it != *b_it || *a_it.flip() != *b_it.flip()) {
        return false;
      }
    }
    return a_it == a.end_left() && b_it == b.end_left();
  }

  friend bool operator!=(bimap const& a, bimap const& b) {
    return !operator==(a, b);
  }

  void swap(bimap& other) noexcept {
    std::swap(get_left_tree(), other.get_left_tree());
    std::swap(get_right_tree(), other.get_right_tree());
    std::swap(sz, other.sz);
  }
};
