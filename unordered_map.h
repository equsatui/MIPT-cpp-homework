#ifndef _UNORDERED_MAP_H_
#define _UNORDERED_MAP_H_

#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
struct ListBaseNode;

template <typename T>
struct ListNode;

template <typename T>
struct ListBaseNode {
 public:
  ListBaseNode<T>* prev;
  ListBaseNode<T>* next;
  ListBaseNode() : prev(this), next(this) {}
  ListBaseNode(const ListBaseNode& other) = delete;
  //    : prev(other.prev), next(other.next) {}
  ListBaseNode(ListBaseNode&& other)
      : prev(other.prev == &other ? this : other.prev),
        next(other.next == &other ? this : other.next) {
    other.prev = &other;
    other.next = &other;
    prev->next = this;
    next->prev = this;
  }
  ListBaseNode& operator=(const ListBaseNode& other) = delete;
  ListBaseNode& operator=(ListBaseNode&& other) = delete;
  /*
  {
    if (other.next == &other) {
      next = this;
      prev = this;
      return *this;
    }
    next = other.next;
    prev = other.prev;
    return *this;
  }
  */
  ListBaseNode(ListBaseNode<T>* prev, ListBaseNode<T>* next)
      : prev(prev), next(next) {}
  T& GetValueRefrence() { return GetNodePointer()->value; }
  T* GetValuePointer() { return &(GetNodePointer()->value); }
  ListNode<T>* GetNodePointer() { return reinterpret_cast<ListNode<T>*>(this); }
};

template <typename T>
struct ListNode : public ListBaseNode<T> {
 protected:
 public:
  T value;
  ListNode(const T& value) : ListBaseNode<T>(nullptr, nullptr), value(value) {}
  ListNode(T&& value)
      : ListBaseNode<T>(nullptr, nullptr), value(std::move(value)) {}
  ListNode(ListNode&& other)
      : ListBaseNode<T>(std::move(other)), value(std::move(other)){};
  ListNode(const ListNode& other) = delete;
  //    : ListBaseNode<T>(other), value(other.value) {}
  ListNode() : ListBaseNode<T>(nullptr, nullptr) {}
  ListNode& operator=(const ListNode& other) = delete;
  ListNode& operator=(ListNode&& other) = delete;
  /*
  {
    ListBaseNode<T>* bother = other.GetListBaseNodePointer();
    value = std::move(other.value);
    if (ListBaseNode<T>::next != GetListBaseNodePointer()) {
      ListBaseNode<T>::next->prev = ListBaseNode<T>::prev;
      ListBaseNode<T>::prev->next = ListBaseNode<T>::next;
    }
    if (bother->next == bother) {
      ListBaseNode<T>::next = this;
      ListBaseNode<T>::prev = this;
      return *this;
    }
    ListBaseNode<T>::next = bother->next;
    ListBaseNode<T>::prev = bother->prev;
    return *this;
  }
  */
  ListBaseNode<T>* GetListBaseNodePointer() {
    return reinterpret_cast<ListBaseNode<T>*>(this);
  }
  T* GetValuePointer() { return &value; }
};

template <typename T, bool IsConst>
struct ListIterator {
  using difference_type = int64_t;
  using value_type = std::conditional_t<IsConst, const T, T>;
  using pointer = std::conditional_t<IsConst, const T*, T*>;
  using reference = std::conditional_t<IsConst, const T&, T&>;
  using iterator_category = std::bidirectional_iterator_tag;

  ListBaseNode<T>* cur;
  ListIterator(const ListBaseNode<T>* node)
      : cur(const_cast<ListBaseNode<T>*>(node)) {}
  ListIterator(const ListBaseNode<T>& node)
      : cur(const_cast<ListBaseNode<T>*>(&node)) {}
  // ListIterator(const ListIterator& other) = default;
  ListIterator(const ListIterator<T, false>& other) : cur(other.cur){};
  ListIterator<T, IsConst>& operator++() {
    cur = cur->next;
    return *this;
  }
  ListIterator<T, IsConst>& operator--() {
    cur = cur->prev;
    return *this;
  }
  ListIterator<T, IsConst> operator++(int) {
    ListIterator<T, IsConst> copy(*this);
    ++(*this);
    return copy;
  }
  ListIterator<T, IsConst> operator--(int) {
    ListIterator<T, IsConst> copy(*this);
    --(*this);
    return copy;
  }
  ListBaseNode<T>* GetNodePtr() { return cur; }
  const ListBaseNode<T>* GetNodePtr() const { return cur; }
  reference operator*() { return cur->GetValueRefrence(); }
  pointer operator->() { return cur->GetValuePointer(); }
  const T& operator*() const { return *cur; }
  const T* operator->() const { return cur; }
  ~ListIterator(){};
};

template <typename T, bool IsConst>
bool operator==(ListIterator<T, IsConst> lhs, ListIterator<T, IsConst> rhs) {
  return lhs.cur == rhs.cur;
}
template <typename T, bool IsConst>
bool operator!=(ListIterator<T, IsConst> lhs, ListIterator<T, IsConst> rhs) {
  return !(lhs == rhs);
}

template <typename T, typename Allocator = std::allocator<T>>
class List {
 private:
  using _node_allocator_t = typename std::allocator_traits<
      Allocator>::template rebind_alloc<ListNode<T>>;
  using _node_traits_t = typename std::allocator_traits<
      Allocator>::template rebind_traits<ListNode<T>>;
  ListBaseNode<T> _fake;
  size_t _size = 0;
  [[no_unique_address]] _node_allocator_t _node_allocator;

 public:
  using iterator = ListIterator<T, false>;
  using const_iterator = ListIterator<T, true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  void easy_swap(List& other) {
    std::swap(_size, other._size);
    std::swap(_node_allocator, other._node_allocator);
    // std::swap(_fake, other._fake);
    ListBaseNode<T>* copy_next = _fake.next;
    ListBaseNode<T>* copy_prev = _fake.prev;
    if (other._fake.next != &other._fake) {
      _fake.next = other._fake.next;
      other._fake.next->prev = &_fake;
      _fake.prev = other._fake.prev;
      other._fake.prev->next = &_fake;
    } else {
      _fake.next = &_fake;
      _fake.prev = &_fake;
    }
    if (copy_next != &_fake) {
      other._fake.next = copy_next;
      copy_next->prev = &other._fake;
      other._fake.prev = copy_prev;
      copy_prev->next = &other._fake;
    } else {
      other._fake.next = &other._fake;
      other._fake.prev = &other._fake;
    }
  }

  List() {}
  List(List&& other)
      : _fake(std::move(other._fake)),
        _size(other._size),
        _node_allocator(other._node_allocator) {
    other._size = 0;
  }
  List(size_t count) {
    for (size_t i = 0; i < count; ++i) {
      ListNode<T>* ptr = _node_traits_t::allocate(_node_allocator, 1);
      try {
        _node_traits_t::construct(_node_allocator, ptr);
      } catch (...) {
        _node_traits_t::deallocate(_node_allocator, ptr, 1);
        this->~List();
        throw;
      }
      ListBaseNode<T>* bptr = ptr->GetListBaseNodePointer();
      bptr->next = begin().cur->next;
      bptr->prev = begin().cur;
      begin().cur->next = bptr;
      bptr->next->prev = bptr;
      ++_size;
    }
  }
  List(size_t count, const T& value) {
    try {
      for (size_t i = 0; i < count; ++i) {
        push_back(value);
      }
    } catch (...) {
      this->~List();
      throw;
    }
  }
  List(Allocator alloc) : _node_allocator(alloc) {}
  List(size_t count, Allocator alloc) : _node_allocator(alloc) {
    for (size_t i = 0; i < count; ++i) {
      ListNode<T>* ptr = _node_traits_t::allocate(_node_allocator, 1);
      try {
        _node_traits_t::construct(_node_allocator, ptr);
      } catch (...) {
        _node_traits_t::deallocate(_node_allocator, ptr, 1);
        this->~List();
        throw;
      }
      ListBaseNode<T>* bptr = ptr->GetListBaseNodePointer();
      bptr->next = begin().cur->next;
      bptr->prev = begin().cur;
      begin().cur->next = bptr;
      bptr->next->prev = bptr;
      ++_size;
    }
  }
  List(size_t count, const T& value, Allocator alloc) : _node_allocator(alloc) {
    try {
      for (size_t i = 0; i < count; ++i) {
        push_back(value);
      }
    } catch (...) {
      this->~List();
      throw;
    }
  }
  ~List() {
    ListBaseNode<T>* bptr = _fake.next;
    for (size_t i = 0; i < _size; ++i) {
      ListBaseNode<T>* next = bptr->next;
      _node_traits_t::destroy(_node_allocator, bptr->GetNodePointer());
      _node_traits_t::deallocate(_node_allocator, bptr->GetNodePointer(), 1);
      bptr = next;
    }
  }
  List(const List& other)
      : _node_allocator(_node_traits_t::select_on_container_copy_construction(
            other._node_allocator)) {
    try {
      for (const auto& i : other) {
        push_back(i);
      }
    } catch (...) {
      this->~List();
      throw;
    }
  }
  List(const List<T, Allocator>& other, Allocator alloc)
      : _node_allocator(alloc) {
    try {
      for (const auto& i : other) {
        push_back(i);
      }
    } catch (...) {
      this->~List();
      throw;
    }
  }
  List<T, Allocator>& operator=(const List<T, Allocator>& other) {
    if constexpr (_node_traits_t::propagate_on_container_copy_assignment::
                      value) {
      List<T, Allocator> copy(other, other._node_allocator);
      easy_swap(copy);
    } else {
      List<T, Allocator> copy(
          other, _node_traits_t::select_on_container_copy_construction(
                     other._node_allocator));
      easy_swap(copy);
    }
    return *this;
  }
  List<T, Allocator>& operator=(List<T, Allocator>&& other) {
    /*
    if constexpr (_node_traits_t::propagate_on_container_move_assignment::
                      value) {
      _node_allocator = other._node_allocator;
    } else {
      //?????
      _node_allocator = _node_traits_t::select_on_container_copy_construction(
          other._node_allocator);
    }*/
    List<T, Allocator> copy(std::move(other));
    easy_swap(copy);
    return *this;
  }
  Allocator get_allocator() { return _node_allocator; }

  template <typename... Args>
  void emplace(const_iterator iter, Args&&... args) {
    ListNode<T>* ptr = _node_traits_t::allocate(_node_allocator, 1);
    try {
      _node_traits_t::construct(_node_allocator, ptr,
                                std::forward<Args>(args)...);
    } catch (...) {
      _node_traits_t::deallocate(_node_allocator, ptr, 1);
      throw;
    }
    ListBaseNode<T>* bptr = ptr->GetListBaseNodePointer();
    bptr->prev = iter.cur->prev;
    bptr->next = iter.cur;
    bptr->prev->next = bptr;
    bptr->next->prev = bptr;
    ++_size;
  }

  template <typename... Args>
  void emplace_back(Args&&... args) {
    return emplace(cend(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  void emplace_front(Args&&... args) {
    return emplace(cbegin(), std::forward<Args>(args)...);
  }

  void insert(const_iterator iter, const T& value) { emplace(iter, value); }

  void insert(const_iterator iter, T&& value) {
    emplace(iter, std::move(value));
  }

  void unlink_node(ListBaseNode<T>* ptr) {
    ptr->prev->next = ptr->next;
    ptr->next->prev = ptr->prev;
    ptr->next = nullptr;
    ptr->prev = nullptr;
    --_size;
  }
  void link_node(iterator iter, ListBaseNode<T>* ptr) {
    ptr->prev = iter.cur->prev;
    ptr->next = iter.cur;
    if (ptr->prev != nullptr) {
      ptr->prev->next = ptr;
    }
    ptr->next->prev = ptr;
    ++_size;
  }

  void push_back(const T& value) { insert(end(), value); }
  void push_front(const T& value) { insert(begin(), value); }
  void erase(const_iterator iter) {
    iter.cur->prev->next = iter.cur->next;
    iter.cur->next->prev = iter.cur->prev;
    ListBaseNode<T>* bptr = iter.cur;
    _node_traits_t::destroy(_node_allocator, bptr->GetNodePointer());
    _node_traits_t::deallocate(_node_allocator, bptr->GetNodePointer(), 1);
    --_size;
  }
  void pop_back() { erase(--end()); }
  void pop_front() { erase(begin()); }
  size_t size() const { return _size; }
  iterator begin() { return iterator(_fake.next); }
  iterator end() { return iterator(_fake); }
  const_iterator cend() const { return const_iterator(_fake); }
  const_iterator cbegin() const { return const_iterator(_fake.next); }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
  reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
};

template <typename Key, typename Value>
struct MapNode {
  using MapPair = std::pair<const Key, Value>;
  // MapPair* pair;
  MapNode(const MapNode& other) = default;
  MapNode(MapNode&& other) = default;
  std::array<char, sizeof(MapPair)> pair;
  size_t hash;
  size_t GetHash() const { return hash; }
  MapPair& GetPairReference() {
    return reinterpret_cast<MapPair&>(*pair.data());
  }
  MapPair* GetPairPointer() { return reinterpret_cast<MapPair*>(pair.data()); }
};

template <typename Key, typename Value, bool IsConst>
struct UnorderedMapIterator {
  using MapNodeType = MapNode<Key, Value>;
  using ListBaseNodeType = ListBaseNode<MapNodeType>;
  using LisNodeType = ListNode<MapNodeType>;
  using MapPair = std::pair<const Key, Value>;

  using difference_type = int64_t;
  using value_type = std::conditional_t<IsConst, const MapPair, MapPair>;
  using pointer = std::conditional_t<IsConst, const MapPair*, MapPair*>;
  using reference = std::conditional_t<IsConst, const MapPair&, MapPair&>;
  using iterator_category = std::bidirectional_iterator_tag;

  ListBaseNodeType* cur;
  UnorderedMapIterator(const ListBaseNodeType* node)
      : cur(const_cast<ListBaseNodeType*>(node)) {}
  UnorderedMapIterator(const ListBaseNodeType& node)
      : cur(const_cast<ListBaseNodeType*>(&node)) {}

  template <bool Const = IsConst, typename std::enable_if_t<Const, int> = 0>
  UnorderedMapIterator(const UnorderedMapIterator<Key, Value, false>& other)
      : cur(other.cur){};

  UnorderedMapIterator(const UnorderedMapIterator& other) = default;

  UnorderedMapIterator<Key, Value, IsConst>& operator++() {
    cur = cur->next;
    return *this;
  }
  UnorderedMapIterator<Key, Value, IsConst>& operator--() {
    cur = cur->prev;
    return *this;
  }
  UnorderedMapIterator<Key, Value, IsConst> operator++(int) {
    UnorderedMapIterator<Key, Value, IsConst> copy(*this);
    ++(*this);
    return copy;
  }
  UnorderedMapIterator<Key, Value, IsConst> operator--(int) {
    UnorderedMapIterator<Key, Value, IsConst> copy(*this);
    --(*this);
    return copy;
  }
  size_t get_hash() const { return cur->GetValuePointer()->hash; }
  reference operator*() { return cur->GetValuePointer()->GetPairReference(); }
  pointer operator->() { return cur->GetValuePointer()->GetPairPointer(); }
  const MapPair& operator*() const {
    return *cur->GetValuePointer()->GetPairReference();
  }
  const MapPair* operator->() const {
    return cur->GetValuePointer()->GetPairPointer();
  }
  ~UnorderedMapIterator(){};
};

template <typename Key, typename Value, bool IsConst>
bool operator==(UnorderedMapIterator<Key, Value, IsConst> lhs,
                UnorderedMapIterator<Key, Value, IsConst> rhs) {
  return lhs.cur == rhs.cur;
}
template <typename Key, typename Value, bool IsConst>
bool operator!=(UnorderedMapIterator<Key, Value, IsConst> lhs,
                UnorderedMapIterator<Key, Value, IsConst> rhs) {
  return !(lhs == rhs);
}

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>,
          typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
 private:
  using MapPair = std::pair<const Key, Value>;
  using MapPairAlloc = Alloc;
  using MapPairAllocTraits = typename std::allocator_traits<MapPairAlloc>;

  using MapNodeType = MapNode<Key, Value>;
  using MapNodeAlloc =
      typename MapPairAllocTraits::template rebind_alloc<MapNodeType>;
  using MapNodeAllocTraits = typename std::allocator_traits<MapNodeAlloc>;

  using ListBaseNodeType = ListBaseNode<MapNodeType>;
  using ListNodeType = ListNode<MapNodeType>;
  using ListNodeAlloc =
      typename MapPairAllocTraits::template rebind_alloc<ListNodeType>;
  using ListNodeAllocTraits = typename std::allocator_traits<ListNodeAlloc>;
  using ListType = List<MapNodeType, MapNodeAlloc>;

  using VectorAlloc =
      typename MapPairAllocTraits::template rebind_alloc<ListBaseNodeType*>;
  using VectorType = std::vector<ListBaseNodeType*, VectorAlloc>;

  static constexpr size_t _default_bucket_count = 32;
  static constexpr float _default_max_load_factor = 0.5;
  float _max_load_factor = _default_max_load_factor;
  VectorType _buckets;
  ListType _list;
  [[no_unique_address]] MapPairAlloc _allocator;

  void easy_swap(UnorderedMap& other) {
    std::swap(_buckets, other._buckets);
    _list.easy_swap(other._list);
    std::swap(_allocator, other._allocator);
    std::swap(_max_load_factor, other._max_load_factor);
  }

 public:
  using iterator = UnorderedMapIterator<Key, Value, false>;
  using const_iterator = UnorderedMapIterator<Key, Value, true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  UnorderedMap() : _buckets(_default_bucket_count, nullptr) {}
  UnorderedMap(const UnorderedMap& other)
      : _max_load_factor(other._max_load_factor),
        _buckets(other._buckets.size(), nullptr),
        _allocator(MapPairAllocTraits::select_on_container_copy_construction(
            other._allocator)) {
    try {
      for (auto it = other.begin(); it != other.end(); ++it) {
        emplace(*it);
      }
    } catch (...) {
      this->~UnorderedMap();
      throw;
    }
  }
  UnorderedMap(UnorderedMap&& other)
      : _max_load_factor(other._max_load_factor),
        _buckets(std::move(other._buckets)),
        _list(std::move(other._list)),
        _allocator(other._allocator) {
    other._buckets = VectorType(_default_bucket_count, nullptr);
  }

  UnorderedMap& operator=(const UnorderedMap& other) {
    UnorderedMap copy(other);
    easy_swap(copy);
    return *this;
  }

  UnorderedMap& operator=(UnorderedMap&& other) {
    UnorderedMap copy(std::move(other));
    easy_swap(copy);
    return *this;
  }

  const_iterator find(const Key& key) const {
    size_t hash = Hash()(key);
    auto list_node_ptr = _buckets[hash % _buckets.size()];
    if (list_node_ptr == nullptr) {
      return end();
    }
    const_iterator iter(list_node_ptr);
    while (iter != end() && iter.get_hash() == hash && !Equal()(key, *iter)) {
      ++iter;
    }
    if (iter == end() || iter.get_hash() != hash) {
      return end();
    }
    return iter;
  }

  iterator find(const Key& key) {
    size_t hash = Hash()(key);
    auto list_node_ptr = _buckets[hash % _buckets.size()];
    if (list_node_ptr == nullptr) {
      return end();
    }
    iterator iter(list_node_ptr);
    while (iter != end() && iter.get_hash() == hash &&
           !Equal()(key, iter->first)) {
      ++iter;
    }
    if (iter == end() || iter.get_hash() != hash) {
      return end();
    }
    return iter;
  }

  Value& at(const Key& key) {
    auto it = find(key);
    if (it == end()) {
      throw std::out_of_range("no key in map");
    }
    return it->second;
  }

  Value& operator[](const Key& key) {
    auto it = find(key);
    if (it != end()) {
      return it->second;
    }
    it = emplace(key, std::move(Value()));
    return it->second;
  }

  Value& operator[](Key&& key) {
    auto it = find(key);
    if (it != end()) {
      return it->second;
    }
    it = emplace(std::move(key), std::move(Value())).first;
    return it->second;
  }

  const Value& at(const Key& key) const {
    auto it = find(key);
    if (it == end()) {
      throw std::out_of_range("no key in map");
    }
    return it->second;
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    ListNodeAlloc list_node_alloc = _allocator;
    ListNodeType* list_node_ptr =
        ListNodeAllocTraits::allocate(list_node_alloc, 1);
    MapNodeType* map_node_ptr = list_node_ptr->GetValuePointer();
    MapPair* pair_ptr = list_node_ptr->GetValuePointer()->GetPairPointer();
    try {
      MapPairAllocTraits::construct(_allocator, pair_ptr,
                                    std::forward<Args>(args)...);
    } catch (...) {
      ListNodeAllocTraits::deallocate(list_node_alloc, list_node_ptr, 1);
      throw;
    }
    size_t hash = Hash()(pair_ptr->first);
    map_node_ptr->hash = hash;
    auto next_list_node_ptr = _buckets[hash % _buckets.size()];
    if (next_list_node_ptr == nullptr) {
      _list.link_node(_list.end(), list_node_ptr);
      _buckets[hash % _buckets.size()] = (--end()).cur;
      return {--end(), true};
    } else {
      iterator iter(next_list_node_ptr);
      while (iter != end() && iter.get_hash() == hash &&
             !Equal()(pair_ptr->first, iter->first)) {
        ++iter;
      }
      if (iter == end() || iter.get_hash() != hash) {
        _list.link_node(iter.cur, list_node_ptr);
      } else {
        MapPairAllocTraits::destroy(_allocator, pair_ptr);
        ListNodeAllocTraits::deallocate(list_node_alloc, list_node_ptr, 1);
        return {end(), false};
      }
      return {--iter, true};
    }
  }

  std::pair<iterator, bool> insert(const MapPair& pair) {
    return emplace(pair);
  }

  std::pair<iterator, bool> insert(MapPair&& pair) {
    return emplace(std::move(pair));
  }

  template <typename X>
  std::pair<iterator, bool> insert(X&& value) {
    return emplace(std::move(value));
  }

  template <typename InputIterator>
  void insert(InputIterator begin, InputIterator end) {
    while (begin != end) {
      emplace(std::forward<decltype(*begin)>(*begin));
      ++begin;
    }
  }

  template <typename InputIterator>
  void erase(InputIterator begin, InputIterator end) {
    InputIterator next = begin;
    ++next;
    while (begin != end) {
      erase(begin);
      begin = next;
      ++next;
    }
  }

  void print() {
    for (auto it = begin(); it != end(); ++it) {
      // std::cout << it->first << ' ' << it->second << std::endl;
      std::cout << 1 << ' ' << std::endl;
    }
  }

  void max_load_factor(float factor) { _max_load_factor = factor; }

  float max_load_factor() const { return _max_load_factor; }

  float load_factor() const { return size() / _buckets.size(); }

  void reserve(size_t count) {
    if (count > std::ceil(_max_load_factor * _buckets.size())) {
      rehash(std::ceil(count / _max_load_factor));
    }
  };

  void rehash(size_t new_bucket_size) {
    if (new_bucket_size < _buckets.size()) {
      return;  //??
    }
    VectorType new_buckets(new_bucket_size, nullptr);
    for (iterator it = begin(); it != end(); ++it) {
      _list.unlink_node(it.cur);
      if (new_buckets[it.get_hash() % new_bucket_size] == nullptr) {
        _list.link_node(_list.end(), it.cur);
        new_buckets[it.get_hash() % new_bucket_size] = (--_list.end()).cur;
      } else {
        _list.link_node(new_buckets[it.get_hash() % new_bucket_size], it.cur);
      }
    }
    _buckets = new_buckets;
  }

  void erase(const_iterator iter) {
    size_t hash = iter.get_hash();
    bool need_to_delete = true;
    if (iter != cbegin()) {
      if (((--iter).get_hash() % _buckets.size()) == (hash % _buckets.size())) {
        need_to_delete = false;
      }
      ++iter;
    }
    if (need_to_delete && (++iter != cend()) &&
        ((iter.get_hash() % _buckets.size()) == (hash % _buckets.size()))) {
      need_to_delete = false;
      _buckets[hash % _buckets.size()] = iter.cur;
    }
    --iter;

    _list.unlink_node(iter.cur);
    MapPairAllocTraits::destroy(_allocator, &(*iter));
    ListNodeAlloc list_node_alloc = _allocator;
    ListNodeAllocTraits::deallocate(list_node_alloc, iter.cur->GetNodePointer(),
                                    1);

    if (need_to_delete) {
      _buckets[hash % _buckets.size()] = nullptr;
    }
  }

  void swap(UnorderedMap& other) {
    if constexpr (MapPairAllocTraits::propagate_on_container_swap::value) {
      _allocator.swap(other._allocator);
    } else {
      std::swap(_allocator, other._allocator);
    }
    std::swap(_list, other._list);
  }

  size_t size() { return _list.size(); }

  iterator begin() { return iterator(_list.begin().GetNodePtr()); }
  iterator end() { return iterator(_list.end().GetNodePtr()); }
  const_iterator cend() const {
    return const_iterator(_list.end().GetNodePtr());
  }
  const_iterator cbegin() const {
    return const_iterator(_list.begin().GetNodePtr());
  }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
  reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  ~UnorderedMap() {
    iterator it = begin();
    for (size_t i = 0; i < _list.size(); ++i) {
      MapPairAllocTraits::destroy(_allocator, &(*it));
      ++it;
    }
  }
};

/*
int main() {
  List<int> lst;
  lst.push_baczk(1);
  lst.push_back(1);
  lst.push_back(1);
  List<int> second_list(std::move(lst));
  UnorderedMap<int, int> map;
  // auto it = map.find(1);
  map[1] = 2;
  map[10] = 100;
  map[2];
  UnorderedMap<int, int> map_second = std::move(map);
  map_second = std::move(map);
  map.print();
  map_second.print();
}*/

#endif