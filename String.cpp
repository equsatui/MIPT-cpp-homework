#include <algorithm>
#include <cstring>
#include <iostream>

class String {
 private:
  void addCapacity(size_t additional_size = 1) {
    if (capacity_ < size_ + additional_size) {
      capacity_ = std::max(2 * capacity_, size_ + additional_size);
      char* new_str = new char[capacity_ + 1];
      std::copy(str_, str_ + size_, new_str);
      new_str[size_] = '\0';
      delete[] str_;
      str_ = new_str;
    }
  }
  size_t size_;
  size_t capacity_;
  char* str_;

 public:
  String(const char* other)
      : size_(std::strlen(other)), capacity_(size_), str_(new char[size_ + 1]) {
    std::copy(other, other + size_, str_);
    str_[size_] = '\0';
  }
  String(char value) : size_(1), capacity_(1), str_(new char[2]) {
    str_[0] = value;
    str_[1] = '\0';
  }
  String(size_t size_, char value)
      : size_(size_), capacity_(size_), str_(new char[size_ + 1]) {
    std::fill(str_, str_ + size_, value);
    str_[size_] = '\0';
  }
  String() : size_(0), capacity_(0), str_(nullptr) {}
  String(const String& other)
      : size_(other.size_), capacity_(size_), str_(new char[size_ + 1]) {
    std::copy(other.str_, other.str_ + other.size_, str_);
    str_[size_] = '\0';
  }
  ~String() { delete[] str_; }
  String& operator=(const String& other) {
    if (str_ == other.str_) {
      return *this;
    }
    String tmp(other);
    swap(tmp);
    return *this;
  }
  size_t length() const { return size_; };
  void swap(String& other) {
    std::swap(other.size_, size_);
    std::swap(other.str_, str_);
    std::swap(other.capacity_, capacity_);
  }
  char* data() { return str_; }
  const char* data() const { return str_; }
  char& operator[](size_t index) { return str_[index]; }
  const char& operator[](size_t index) const { return str_[index]; }
  void push_back(char value) {
    addCapacity();
    str_[size_] = value;
    str_[size_ + 1] = '\0';
    ++size_;
  }
  void pop_back() {
    if (size_ == 0) {
      return;
    }
    --size_;
    str_[size_] = '\0';
  }
  char& front() { return str_[0]; }
  char& back() { return str_[size_ - 1]; }
  const char& front() const { return str_[0]; }
  const char& back() const { return str_[size_ - 1]; }
  String& operator+=(const String& other) {
    addCapacity(other.size_);
    std::copy(other.str_, other.str_ + other.size_, str_ + size_);
    size_ += other.size_;
    str_[size_] = '\0';
    return *this;
  }
  size_t find(const String& substring) const {
    for (size_t i = 0; i < size_; ++i) {
      if (i + substring.size_ > size_) {
        return size_;
      }
      if (std::strncmp(str_ + i, substring.str_, substring.size_) == 0) {
        return i;
      }
    }
    return size_;
  }
  size_t rfind(const String& substring) const {
    if (size_ == 0) {
      return size_;
    }
    size_t i = size_;
    do {
      --i;
      if (i + substring.size_ <= size_) {
        if (std::strncmp(str_ + i, substring.str_, substring.size_) == 0) {
          return i;
        }
      }
    } while (i != 0);
    return size_;
  }
  String substr(size_t start, size_t count) const {
    String result;
    result.str_ = new char[count + 1];
    if (start + count > size_) {
      count = size_ - start;
    }
    std::copy(str_ + start, str_ + start + count, result.str_);
    result.str_[count] = '\0';
    result.size_ = count;
    result.capacity_ = count;
    return result;
  }
  bool empty() const { return size_ == 0; }
  void clear() {
    if (str_ != nullptr) {
      size_ = 0;
      str_[0] = '\0';
    }
  }
  void shrink_to_fit() {
    if (capacity_ == size_) {
      return;
    }
    char* new_str = new char[size_ + 1];
    std::copy(str_, str_ + size_, new_str);
    new_str[size_] = '\0';
    delete[] str_;
    str_ = new_str;
    capacity_ = size_;
  }
  size_t size() const { return size_; }
  size_t capacity() const { return capacity_; }
  friend std::ostream& operator<<(std::ostream& out, const String& string);
};

bool operator==(const String& first, const String& second) {
  if (first.length() != second.length()) {
    return false;
  }
  return std::strcmp(first.data(), second.data()) == 0;
}

bool operator!=(const String& first, const String& second) {
  return !(first == second);
}

bool operator<(const String& first, const String& second) {
  return std::strcmp(first.data(), second.data()) < 0;
}

bool operator>(const String& first, const String& second) {
  return second < first;
}

bool operator<=(const String& first, const String& second) {
  return !(second < first);
}

bool operator>=(const String& first, const String& second) {
  return !(first < second);
}

String operator+(String first, const String& second) {
  first += second;
  return first;
}

std::ostream& operator<<(std::ostream& out, const String& string) {
  out << string.str_;
  return out;
}

std::istream& operator>>(std::istream& in, String& string) {
  char value;
  string.clear();
  string.shrink_to_fit();
  while (in.get(value) && !std::isspace(value)) {
    string.push_back(value);
  }
  return in;
}
