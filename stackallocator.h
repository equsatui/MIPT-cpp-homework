#ifndef _LIST_H_
#define _LIST_H_

#include <array>
#include <iostream>
#include <memory>
#include <type_traits>

template <size_t N>
struct StackStorage {
  std::array<char, N> array;
  size_t shift;
  StackStorage() : shift(0) {}
  StackStorage(const StackStorage& other) = delete;
  StackStorage& operator=(const StackStorage& other) = delete;
};

template <typename T, size_t N>
struct StackAllocator {
 private:
  void swap(StackAllocator<T, N>& other) { std::swap(stack, other.stack); }

 public:
  using value_type = T;
  using type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  StackStorage<N>* stack;
  StackAllocator(StackStorage<N>& ref) : stack(&ref) {}
  template <typename U>
  StackAllocator(const StackAllocator<U, N>& other) : stack(other.stack) {}

  template <typename U>
  StackAllocator<T, N> operator=(const StackAllocator<U, N>& other) {
    StackAllocator<T, N> copy(other);
    swap(copy);
    return *this;
  }
  ~StackAllocator() = default;

  T* allocate(size_t count) {
    size_t space = N - stack->shift;
    void* cur = static_cast<void*>(stack->array.data() + stack->shift);
    void* old = cur;
    std::align(alignof(T), sizeof(T), cur, space);
    stack->shift +=
        count * sizeof(T) + (static_cast<char*>(cur) - static_cast<char*>(old));
    return static_cast<T*>(cur);
  }
  void deallocate(T*, size_t) {}
  // construct, destroy -> default
  template <typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };
};

template <typename T>
struct BaseNode;

template <typename T>
struct Node;

template <typename T>
struct BaseNode {
  BaseNode<T>* prev;
  BaseNode<T>* next;
  BaseNode() : prev(this), next(this) {}
  BaseNode(BaseNode<T>* prev, BaseNode<T>* next) : prev(prev), next(next) {}
  T& GetValueRefrence() { return GetNodePointer()->value; }
  T* GetValuePointer() { return &(GetNodePointer()->value); }
  Node<T>* GetNodePointer() { return reinterpret_cast<Node<T>*>(this); }
};

template <typename T>
struct Node : private BaseNode<T> {
  T value;
  Node(const T& value) : BaseNode<T>(nullptr, nullptr), value(value) {}
  Node() : BaseNode<T>(nullptr, nullptr) {}
  BaseNode<T>* GetBaseNodePointer() {
    return reinterpret_cast<BaseNode<T>*>(this);
  }
};

template <typename T, bool IsConst>
struct ListIterator {
  using difference_type = int64_t;
  using value_type = std::conditional_t<IsConst, const T, T>;
  using pointer = std::conditional_t<IsConst, const T*, T*>;
  using reference = std::conditional_t<IsConst, const T&, T&>;
  using iterator_category = std::bidirectional_iterator_tag;

  BaseNode<T>* cur;
  ListIterator(const BaseNode<T>* node) : cur(const_cast<BaseNode<T>*>(node)) {}
  ListIterator(const BaseNode<T>& node)
      : cur(const_cast<BaseNode<T>*>(&node)) {}
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
  using _node_allocator_t =
      typename std::allocator_traits<Allocator>::template rebind_alloc<Node<T>>;
  using _node_traits_t = typename std::allocator_traits<
      Allocator>::template rebind_traits<Node<T>>;
  BaseNode<T> _fake;
  size_t _size = 0;
  [[no_unique_address]] _node_allocator_t _node_allocator;

  void swap(List<T, Allocator>& other) {
    std::swap(_size, other._size);
    std::swap(_fake, other._fake);
    auto copy(other._node_allocator);
    other._node_allocator = _node_allocator;
    _node_allocator = copy;
  }

 public:
  using iterator = ListIterator<T, false>;
  using const_iterator = ListIterator<T, true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  List() {}
  List(size_t count) {
    for (size_t i = 0; i < count; ++i) {
      Node<T>* ptr = _node_traits_t::allocate(_node_allocator, 1);
      try {
        _node_traits_t::construct(_node_allocator, ptr);
      } catch (...) {
        _node_traits_t::deallocate(_node_allocator, ptr, 1);
        this->~List();
        throw;
      }
      BaseNode<T>* bptr = ptr->GetBaseNodePointer();
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
      Node<T>* ptr = _node_traits_t::allocate(_node_allocator, 1);
      try {
        _node_traits_t::construct(_node_allocator, ptr);
      } catch (...) {
        _node_traits_t::deallocate(_node_allocator, ptr, 1);
        this->~List();
        throw;
      }
      BaseNode<T>* bptr = ptr->GetBaseNodePointer();
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
    BaseNode<T>* bptr = _fake.next;
    for (size_t i = 0; i < _size; ++i) {
      BaseNode<T>* next = bptr->next;
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
      swap(copy);
    } else {
      List<T, Allocator> copy(other);
      swap(copy);
    }
    return *this;
  }
  Allocator get_allocator() { return _node_allocator; }
  void insert(const_iterator iter, const T& value) {
    Node<T>* ptr = _node_traits_t::allocate(_node_allocator, 1);
    try {
      _node_traits_t::construct(_node_allocator, ptr, value);
    } catch (...) {
      _node_traits_t::deallocate(_node_allocator, ptr, 1);
      throw;
    }
    BaseNode<T>* bptr = ptr->GetBaseNodePointer();
    bptr->prev = iter.cur->prev;
    bptr->next = iter.cur;
    bptr->prev->next = bptr;
    bptr->next->prev = bptr;
    ++_size;
  }
  void push_back(const T& value) { insert(end(), value); }
  void push_front(const T& value) { insert(begin(), value); }
  void erase(const_iterator iter) {
    iter.cur->prev->next = iter.cur->next;
    iter.cur->next->prev = iter.cur->prev;
    BaseNode<T>* bptr = iter.cur;
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
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }
};

#endif