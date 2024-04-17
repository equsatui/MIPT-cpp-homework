#ifndef _DEQUE_H_
#define _DEQUE_H_
#include <iostream>
#include <utility>

template <typename T>
struct DequeIterator;

template <typename T>
class Deque {
 private:
  T** _map;
  size_t _map_begin;
  size_t _map_end;
  size_t _map_capacity;
  size_t _block_begin;
  size_t _block_end;
  static const size_t _block_capacity = 64;
  void IncreaseMap() {
    T** new_map = new T*[_map_capacity * 3];
    for (size_t i = 0; i < _map_capacity; ++i) {
      new_map[i] =
          static_cast<T*>(::operator new[](_block_capacity * sizeof(T)));
      new_map[2 * _map_capacity + i] =
          static_cast<T*>(::operator new[](_block_capacity * sizeof(T)));
    }
    std::copy(_map, _map + _map_capacity, new_map + _map_capacity);
    delete[] _map;
    _map = new_map;
    _map_begin += _map_capacity;
    _map_end += _map_capacity;
    _map_capacity *= 3;
  }

  void CopyElements(size_t index, size_t begin, size_t end, size_t& cur,
                    T** from) {
    for (cur = begin; cur < end; ++cur) {
      new (_map[index] + cur) T(from[index][cur]);
    }
  }

  template <typename... Args>
  void CopyElements(size_t index, size_t begin, size_t end, size_t& cur,
                    Args&&... args) {
    for (cur = begin; cur < end; ++cur) {
      new (_map[index] + cur) T(std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  void CopyInterval(Args&&... args) {  // CopyInterval calls only in constructor
    size_t map_cur = _map_begin;
    size_t block_cur = _block_begin;
    try {
      // copying first block
      if (_map_begin + 1 == _map_end) {
        CopyElements(map_cur, _block_begin, _block_end, block_cur,
                     std::forward<Args>(args)...);
      } else {
        CopyElements(map_cur, _block_begin, _block_capacity, block_cur,
                     std::forward<Args>(args)...);
      }
      ++map_cur;
      // (first, last) blocks
      for (; map_cur + 1 < _map_end; ++map_cur) {
        CopyElements(map_cur, 0, _block_capacity, block_cur,
                     std::forward<Args>(args)...);
      }
      // last block
      if (_map_begin + 1 != _map_end) {
        CopyElements(map_cur, 0, _block_end, block_cur,
                     std::forward<Args>(args)...);
      }
    } catch (...) {
      ++map_cur;
      _map_end = map_cur;
      _block_end = block_cur;
      if (_map_end == _map_begin) {
        ++_map_end;
      }
      this->~Deque();
      throw;
    }
  }

  void DeleteElements(size_t index, size_t begin, size_t end) {
    for (size_t cur = begin; cur < end; ++cur) {
      (_map[index] + cur)->~T();
    }
    ::operator delete[](_map[index]);
  }
  void Swap(Deque<T>& other) {
    std::swap(_map, other._map);
    std::swap(_map_begin, other._map_begin);
    std::swap(_map_end, other._map_end);
    std::swap(_map_capacity, other._map_capacity);
    std::swap(_block_begin, other._block_begin);
    std::swap(_block_end, other._block_end);
  }
  friend DequeIterator<T>;

 public:
  using iterator = DequeIterator<T>;
  using const_iterator = DequeIterator<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  // _map_begin + 1 == _map_end at empty deque
  // _map_end at 1 distance to the last block which can be used in push_back
  // _block_end < _block_capacity
  Deque()
      : _map(new T*[3]),
        _map_begin(1),
        _map_end(2),
        _map_capacity(3),
        _block_begin(0),
        _block_end(0) {
    for (size_t i = 0; i < _map_capacity; ++i) {
      _map[i] = static_cast<T*>(::operator new[](_block_capacity * sizeof(T)));
    }
  }
  Deque(const Deque& other) {
    _map_begin = other._map_begin;
    _map_end = other._map_end;
    _map_capacity = other._map_capacity;
    _block_begin = other._block_begin;
    _block_end = other._block_end;
    _map = new T*[other._map_capacity];
    for (size_t i = 0; i < other._map_capacity; ++i) {
      _map[i] = static_cast<T*>(::operator new[](_block_capacity * sizeof(T)));
    }
    CopyInterval(other._map);
  }
  ~Deque() {
    // deleting [0, first)
    for (size_t i = 0; i < _map_begin; ++i) {
      // delete[] map[i];
      ::operator delete[](_map[i]);
    }
    // deleting first
    if (_map_begin + 1 != _map_end) {
      DeleteElements(_map_begin, _block_begin, _block_capacity);
    } else {
      DeleteElements(_map_begin, _block_begin, _block_end);
    }
    // deleting (first, last)
    for (size_t i = _map_begin + 1; i + 1 < _map_end; ++i) {
      DeleteElements(i, 0, _block_capacity);
    }
    // deleting last
    if (_map_begin + 1 != _map_end) {
      DeleteElements(_map_end - 1, 0, _block_end);
    }
    // deleting (last, end)
    for (size_t i = _map_end; i < _map_capacity; ++i) {
      // delete[] map[i];
      ::operator delete[](_map[i]);
    }
    delete[] _map;
  }
  Deque(int size, const T& value) {
    _map_capacity = size / _block_capacity + 1;
    _map_begin = 0;
    _map_end = size / _block_capacity + 1;
    _block_begin = 0;
    _block_end = size % _block_capacity;
    _map = new T*[_map_capacity];
    for (size_t i = 0; i < _map_capacity; ++i) {
      _map[i] = static_cast<T*>(::operator new[](_block_capacity * sizeof(T)));
    }
    CopyInterval(value);
  }
  template <typename U = T, typename std::enable_if_t<
                                std::is_default_constructible_v<U>, int> = 0>
  Deque(int size) {
    _map_capacity = size / _block_capacity + 1;
    _map_begin = 0;
    _map_end = size / _block_capacity + 1;
    _block_begin = 0;
    _block_end = size % _block_capacity;
    _map = new T*[_map_capacity];
    for (size_t i = 0; i < _map_capacity; ++i) {
      _map[i] = static_cast<T*>(::operator new[](_block_capacity * sizeof(T)));
    }
    CopyInterval();
  }
  Deque<T>& operator=(const Deque<T>& other) {
    Deque<T> copy(other);
    Swap(copy);
    return *this;
  }
  size_t size() const {
    return (_map_end - _map_begin - 1) * _block_capacity + _block_end -
           _block_begin;
  }
  T& operator[](size_t index) {
    // if index at first block:
    if (_block_begin + index < _block_capacity) {
      return _map[_map_begin][_block_begin + index];
    }
    index -= _block_capacity - _block_begin;
    return _map[_map_begin + 1 + index / _block_capacity]
               [index % _block_capacity];
  }
  const T& operator[](size_t index) const {
    // if index at first block:
    if (_block_begin + index < _block_capacity) {
      return _map[_map_begin][_block_begin + index];
    }
    index -= _block_capacity - _block_begin;
    return _map[_map_begin + 1 + index / _block_capacity]
               [index % _block_capacity];
  }
  T& at(size_t index) {
    if (index >= size()) {
      throw std::out_of_range("Deque out of range");
    }
    return (*this)[index];
  }
  const T& at(size_t index) const {
    if (index >= size()) {
      throw std::out_of_range("Deque out of range");
    }
    return (*this)[index];
  }
  void push_back(const T& value) {
    new (_map[_map_end - 1] + _block_end) T(value);
    ++_block_end;
    if (_block_end == _block_capacity) {
      _block_end = 0;
      ++_map_end;
      if (_map_end == _map_capacity + 1) {
        IncreaseMap();
      }
    }
  }
  void pop_back() {
    if (_block_end == 0) {
      _block_end = _block_capacity;
      --_map_end;
    }
    (_map[_map_end - 1] + _block_end - 1)->~T();
    --_block_end;
  }
  void push_front(const T& value) {
    if (_block_begin == 0) {
      if (_map_begin == 0) {
        IncreaseMap();
      }
      _map_begin -= 1;
      _block_begin = _block_capacity - 1;
    } else {
      --_block_begin;
    }
    new (_map[_map_begin] + _block_begin) T(value);
  }
  void pop_front() {
    (_map[_map_begin] + _block_begin)->~T();
    ++_block_begin;
    if (_block_begin == _block_capacity) {
      _block_begin = 0;
      ++_map_begin;
    }
  }
  void erase(iterator iter) {
    iter.cur->~T();
    while (iter + 1 != end()) {
      std::swap(*iter, *(iter + 1));
      ++iter;
    }
    if (_block_end == 0) {
      _block_end = _block_capacity - 1;
      _map_end -= 1;
    } else {
      _block_end -= 1;
    }
  }
  void insert(const iterator iter, const T& value) {
    iterator help = end();
    push_back(value);
    while (help != iter) {
      std::swap(*help, *(help - 1));
      --help;
    }
  }
  void print() const {
    std::cout << std::endl;
    std::cout << size() << ": ";
    for (size_t i = 0; i < size(); ++i) {
      std::cout << (*this)[i] << ", ";
    }
    std::cout << std::endl;
  }
  iterator begin() { return iterator(_map, _map_begin, _block_begin); }
  iterator end() { return iterator(_map, _map_end - 1, _block_end); }
  const_iterator cend() const {
    return const_iterator(const_cast<const T**>(_map), _map_end - 1,
                          _block_end);
  }
  const_iterator cbegin() const {
    return const_iterator(const_cast<const T**>(_map), _map_begin,
                          _block_begin);
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
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }
};

template <typename T>
struct DequeIterator {
  using difference_type = int64_t;
  using value_type = T;
  using pointer = T*;
  using reference = T&;
  using iterator_category = std::random_access_iterator_tag;
  T** map;
  size_t map_cur;
  size_t block_cur;
  static const size_t _block_capacity = Deque<T>::_block_capacity;
  T* cur;
  DequeIterator(T** map, size_t map_cur, size_t block_cur)
      : map(map),
        map_cur(map_cur),
        block_cur(block_cur),
        cur(map[map_cur] + block_cur) {}

  template <typename U = T,
            typename std::enable_if_t<std::is_const_v<U>, int> = 0>
  DequeIterator(const DequeIterator<std::remove_const_t<T>>& other)
      : map(const_cast<T**>(other._map)),
        map_cur(other._map_cur),
        block_cur(other._block_cur),
        cur(const_cast<T*>(other._cur)) {}

  DequeIterator(const DequeIterator& other) = default;
  DequeIterator& operator=(const DequeIterator<T>& other) = default;
  DequeIterator<T>& operator++() {
    ++block_cur;
    if (block_cur == _block_capacity) {
      block_cur = 0;
      ++map_cur;
      cur = map[map_cur];
    } else {
      ++cur;
    }
    return *this;
  }
  DequeIterator<T> operator++(int) {
    DequeIterator<T> copy(*this);
    ++block_cur;
    if (block_cur == _block_capacity) {
      block_cur = 0;
      ++map_cur;
      cur = map[map_cur];
    } else {
      ++cur;
    }
    return copy;
  }
  DequeIterator<T>& operator--() {
    if (block_cur == 0) {
      block_cur = _block_capacity - 1;
      --map_cur;
      cur = map[map_cur] + block_cur;
    } else {
      --block_cur;
      --cur;
    }
    return *this;
  }
  DequeIterator<T> operator--(int) {
    DequeIterator<T> copy(*this);
    if (block_cur == 0) {
      block_cur = _block_capacity - 1;
      --map_cur;
      cur = map[map_cur] + block_cur;
    } else {
      --block_cur;
      --cur;
    }
    return copy;
  }
  DequeIterator<T>& operator+=(int64_t value) {
    if (value >= 0) {
      if (block_cur + value < _block_capacity) {
        block_cur += value;
        cur += value;
      } else {
        value -= _block_capacity - block_cur;
        block_cur = value % _block_capacity;
        map_cur += 1 + value / _block_capacity;
        cur = map[map_cur] + block_cur;
      }
    } else {
      *this += (-value);
    }
    return *this;
  }
  DequeIterator<T>& operator-=(int64_t value) {
    if (value >= 0) {
      int64_t res = static_cast<int64_t>(block_cur) - value;
      if (static_cast<int64_t>(block_cur) >= value) {
        block_cur -= value;
        cur -= value;
      } else {
        map_cur -= (-res) / _block_capacity + 1;
        block_cur = _block_capacity - (-res) % _block_capacity;
        if (block_cur == _block_capacity) {
          block_cur = 0;
          map_cur += 1;
        }
        cur = map[map_cur] + block_cur;
      }
    } else {
      *this += (-value);
    }
    return *this;
  }
  T& operator*() { return *cur; }
  const T& operator*() const { return *cur; }
  const T* operator->() const { return cur; }
  T* operator->() { return cur; }
};

template <typename T>
DequeIterator<T> operator+(DequeIterator<T> iter, int64_t value) {
  iter += value;
  return iter;
}
template <typename T>
DequeIterator<T> operator-(DequeIterator<T> iter, int64_t value) {
  iter -= value;
  return iter;
}
template <typename T>
bool operator==(const DequeIterator<T>& first, const DequeIterator<T>& second) {
  return first.map_cur == second.map_cur && first.block_cur == second.block_cur;
}
template <typename T>
bool operator!=(const DequeIterator<T>& first, const DequeIterator<T>& second) {
  return !(first == second);
}
template <typename T>
bool operator<(const DequeIterator<T>& first, const DequeIterator<T>& second) {
  if (first.map_cur != second.map_cur) {
    return first.map_cur < second.map_cur;
  }
  return first.block_cur < second.block_cur;
}
template <typename T>
bool operator>(const DequeIterator<T>& first, const DequeIterator<T>& second) {
  return second < first;
}
template <typename T>
bool operator<=(const DequeIterator<T>& first, const DequeIterator<T>& second) {
  return first < second || first == second;
}
template <typename T>
bool operator>=(const DequeIterator<T>& first, const DequeIterator<T>& second) {
  return second < first || first == second;
}
template <typename T>
int64_t operator-(const DequeIterator<T>& first,
                  const DequeIterator<T>& second) {  // it's a size of deque..
  return (static_cast<int64_t>(first.map_cur + 1) -
          static_cast<int64_t>(second.map_cur) - 1) *
             static_cast<int64_t>(first._block_capacity) +
         static_cast<int64_t>(first.block_cur) -
         static_cast<int64_t>(second.block_cur);
}

#endif