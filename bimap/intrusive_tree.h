#pragma once

#include <algorithm>
#include <cstdint>
#include <random>

namespace intrusive {

struct tree_element_base {
  // tree work

  tree_element_base() noexcept = default;

  ~tree_element_base() noexcept;

  tree_element_base(tree_element_base const&) = delete;

  tree_element_base(tree_element_base&&) noexcept;

  tree_element_base& operator=(tree_element_base const&) = delete;

  tree_element_base& operator=(tree_element_base&&) noexcept;

  void unlink() noexcept;

  tree_element_base* left{nullptr};
  tree_element_base* right{nullptr};
  tree_element_base* parent{nullptr};
};

struct default_tag;

template <typename Tag = default_tag>
struct tree_element : tree_element_base {};

inline thread_local std::mt19937_64 random_generator{std::random_device()()};

template <typename K, typename Tag = default_tag>
struct node : tree_element<Tag> {
  K key;
  uint64_t priority;

  explicit node(K &&key) noexcept
      : key(std::move(key)), priority(random_generator()) {}

  explicit node(const K &key) noexcept
      : key(key), priority(random_generator()) {}

  node(const node& other) noexcept : key(other.key), priority(other.priority) {}

  node(node&& other) noexcept
      : key(std::move(other.key)), priority(other.priority),
        tree_element_base(static_cast<tree_element_base&&>(std::move(other))) {}

  node& operator=(const node&) = delete;

  node& operator=(node&& other) noexcept {
    if (this != &other) {
      std::swap(key, other.key);
      std::swap(priority, other.priority);
      std::swap(to_base(*this), to_base(other));
    }
    return *this;
  }
};

template <typename K, typename Tag = default_tag>
node<K, Tag>& from_base(tree_element_base& base) noexcept {
  return static_cast<node<K, Tag>&>(base);
}

template <typename K, typename Tag = default_tag>
tree_element_base& to_base(node<K, Tag>& node) noexcept {
  return static_cast<tree_element_base&>(node);
}

template <typename K, typename Comp = std::less<K>, typename Tag = default_tag>
struct intrusive_tree : private Comp, tree_element<Tag> {

  using node_t = node<K, Tag>;
  using elem_t = tree_element<Tag>;

  intrusive_tree(Comp &&comp) noexcept : Comp(std::move(comp)) {}

  intrusive_tree(const intrusive_tree&) = delete;

  intrusive_tree(intrusive_tree&& other) noexcept
      : Comp(std::move(other)), elem_t(std::move(other.to_root())) {
    auto *root_base = &static_cast<tree_element_base&>(to_root());
    update_parent(root_base->left, root_base);
    other.clear();
  }

  intrusive_tree& operator=(const intrusive_tree&) = delete;

  intrusive_tree& operator=(intrusive_tree&& other) noexcept {
    if (this != &other) {
      to_root() = std::move(other.to_root());
      static_cast<Comp&>(*this) = std::move(static_cast<Comp&>(other));
    }
    return *this;
  }

  const Comp &get_comparator() const noexcept {
    return static_cast<const Comp&>(*this);
  }

  template <typename IT = K>
  struct tree_iterator {

    friend intrusive_tree;

    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = std::remove_const_t<K>;
    using reference = value_type&;
    using pointer = value_type*;
    using difference_type = std::ptrdiff_t;

    tree_iterator() noexcept = default;

    explicit tree_iterator(elem_t* elem) noexcept :
          data(static_cast<tree_element_base*>(elem)){};

    template <typename OtherIT>
    tree_iterator(
        tree_iterator<OtherIT> other,
        std::enable_if<std::is_same_v<std::remove_const<IT>, OtherIT> &&
                           std::is_const<IT>::value,
                       OtherIT>* = nullptr) noexcept
        : data(other.data) {}

    tree_iterator& operator++() noexcept {
      if (data->right) {
        data = data->right;
        while (data->left) {
          data = data->left;
        }
      } else {
        while (data->parent && data->parent->right == data) {
          data = data->parent;
        }
        data = data->parent;
      }
      return *this;
    }

    tree_iterator operator++(int) noexcept {
      tree_iterator copy = *this;
      operator++();
      return copy;
    }

    tree_iterator& operator--() noexcept {
      if (data->left) {
        data = data->left;
        while (data->right) {
          data = data->right;
        }
      } else {
        while (data->parent && data->parent->left == data) {
          data = data->parent;
        }
        data = data->parent;
      }
      return *this;
    }

    tree_iterator operator--(int) noexcept {
      tree_iterator copy = *this;
      operator--();
      return copy;
    }

    const IT* operator->() const noexcept {
      return &operator*();
    }

    const IT& operator*() const noexcept {
      return get_node()->key;
    }

    bool operator==(tree_iterator const& other) const noexcept {
      return data == other.data;
    }

    bool operator!=(tree_iterator const& other) const noexcept {
      return !(*this == other);
    }

    node_t* get_node() const noexcept {
      return &const_cast<node_t&>(from_base<K, Tag>(*data));
    }

    elem_t* get_elem() const noexcept {
      return &static_cast<tree_element<Tag>&>(*data);
    }

    tree_element_base* get_base() const noexcept {
      return data;
    }

  private:
    explicit tree_iterator(const tree_element_base* data) noexcept
        : data(const_cast<tree_element_base*>(data)) {}

    tree_element_base* data;
  };

  using iterator = tree_iterator<K>;
  using const_iterator = tree_iterator<const K>;

  iterator find(const K& key) const noexcept {
    return find(to_root().left, key);
  }

  iterator insert(node_t* node) noexcept {
    to_root().left = insert(to_root().left, node);
    update_parent(to_root().left, &to_root());
    return iterator(node);
  }

  iterator erase(const iterator& it) noexcept {
    to_root().left = erase(to_root().left, get_key(it.data));
    update_parent(to_root().left, &to_root());
    return lower_bound(to_root().left, get_key(it.data));
  }

  const tree_element_base* least_element() const noexcept {
    tree_element_base* curr = to_root().left;
    if (!curr) {
      return &to_root();
    }
    while (curr->left != nullptr) {
      curr = curr->left;
    }
    return curr;
  }

  iterator lower_bound(const K& key) const noexcept {
    return lower_bound(to_root().left, key);
  }

  iterator upper_bound(const K& key) const noexcept {
    return upper_bound(to_root().left, key);
  }

  iterator begin() noexcept {
    return iterator(least_element());
  }

  const_iterator begin() const noexcept {
    return const_iterator(least_element());
  }

  iterator end() noexcept {
    return iterator(&to_root());
  }

  const_iterator end() const noexcept {
    return const_iterator(&to_root());
  }

private:

  bool compare(const K& k1, const K& k2) const noexcept {
    return get_comparator()(k1, k2);
  }

  bool equals(const K& k1, const K& k2) const noexcept {
    return !compare(k1, k2) && !compare(k2, k1);
  }

  void clear() {
    while (begin() != end()) {
      erase(begin());
    }
  }

  uint64_t& get_priority(tree_element_base* base) const noexcept {
    return node_t_from_base(base)->priority;
  }

  node_t* node_t_from_base(tree_element_base* base) const noexcept {
    return &const_cast<node_t&>(
        static_cast<const node_t&>(from_base<K, Tag>(*base)));
  }

  const K& get_key(tree_element_base* base) const noexcept {
    return node_t_from_base(base)->key;
  }

  void update_parent(tree_element_base* child,
                     tree_element_base* parent) noexcept {
    if (child) {
      child->parent = parent;
    }
  }

  std::pair<tree_element_base*, tree_element_base*>
  split(tree_element_base* curr, const K& key) noexcept {
    if (!curr) {
      return {nullptr, nullptr};
    }
    if (compare(get_key(curr), key)) {
      std::pair<tree_element_base*, tree_element_base*> res =
          split(curr->right, key);
      curr->right = res.first;
      update_parent(res.second, nullptr);
      update_parent(curr->right, curr);
      return {curr, res.second};
    } else {
      std::pair<tree_element_base*, tree_element_base*> res =
          split(curr->left, key);
      curr->left = res.second;
      update_parent(res.first, nullptr);
      update_parent(curr->left, curr);
      return {res.first, curr};
    }
  }

  tree_element_base* merge(tree_element_base* root1,
                           tree_element_base* root2) noexcept {
    if (!root1) {
      return root2;
    }
    if (!root2) {
      return root1;
    }
    if (get_priority(root1) < get_priority(root2)) {
      root1->right = merge(root1->right, root2);
      update_parent(root1->right, root1);
      return root1;
    } else {
      root2->left = merge(root1, root2->left);
      update_parent(root2->left, root2);
      return root2;
    }
  }

  iterator find(tree_element_base* curr, const K& key) const noexcept {
    if (!curr) {
      return end();
    }
    if (equals(get_key(curr), key)) {
      return iterator(curr);
    }
    if (compare(key, get_key(curr))) {
      return find(curr->left, key);
    }
    return find(curr->right, key);
  }

  tree_element_base* insert(tree_element_base* curr,
                            tree_element_base* v) noexcept {
    if (!curr) {
      return v;
    }
    if (!v) {
      return curr;
    }
    if (get_priority(v) < get_priority(curr)) {
      auto p = split(curr, get_key(v));
      v->left = p.first;
      v->right = p.second;
      update_parent(p.first, v);
      update_parent(p.second, v);
      return v;
    }
    if (compare(get_key(v), get_key(curr))) {
      curr->left = insert(curr->left, v);
      update_parent(curr->left, curr);
    } else {
      curr->right = insert(curr->right, v);
      update_parent(curr->right, curr);
    }
    return curr;
  }

  tree_element_base* erase(tree_element_base* curr, const K& key) noexcept {
    if (!curr) {
      return nullptr;
    }
    if (equals(get_key(curr), key)) {
      auto par = curr->parent;
      auto res = merge(curr->left, curr->right);
      update_parent(res, par);
      return res;
    }
    if (compare(key, get_key(curr))) {
      curr->left = erase(curr->left, key);
      update_parent(curr->left, curr);
    } else {
      curr->right = erase(curr->right, key);
      update_parent(curr->right, curr);
    }
    return curr;
  }

  iterator find_bound(tree_element_base* curr, const K& key,
                       tree_element_base* best, bool strict_bound) const noexcept {
    if (!curr && best) {
      return iterator(best);
    }
    if (!curr) {
      return iterator(end());
    }
    if ((compare(key, get_key(curr)) && !curr->left) ||
        (!strict_bound && equals(key, get_key(curr)))) {
      return iterator(curr);
    }
    if (compare(key, get_key(curr))) {
      return find_bound(curr->left, key, curr, strict_bound);
    }
    return find_bound(curr->right, key, best, strict_bound);
  }

  iterator lower_bound(tree_element_base* curr, const K& key) const noexcept {
      return find_bound(curr, key, nullptr, false);
  }

  iterator upper_bound(tree_element_base* curr, const K& key) const noexcept {
    return find_bound(curr, key, nullptr, true);
  }

  const elem_t& to_root() const noexcept {
    return static_cast<const elem_t&>(*this);
  }

  elem_t& to_root() noexcept {
    return static_cast<elem_t&>(*this);
  }

  // base node_t is a fake node, her .left is real root
};

} // namespace intrusive
