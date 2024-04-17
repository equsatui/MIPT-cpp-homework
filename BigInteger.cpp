#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

size_t count_digits(uint64_t value) {
  size_t count = 0;
  while (value != 0) {
    ++count;
    value /= 10;
  }
  return count;
}

class BigInteger;
std::ostream& operator<<(std::ostream& out, const BigInteger& value);
BigInteger operator-(const BigInteger& first, const BigInteger& second);

bool operator==(const BigInteger& first, const BigInteger& other);
bool operator!=(const BigInteger& first, const BigInteger& other);
bool operator<(const BigInteger& first, const BigInteger& other);
bool operator>(const BigInteger& first, const BigInteger& other);
bool operator>=(const BigInteger& first, const BigInteger& other);
bool operator<=(const BigInteger& first, const BigInteger& other);

class BigInteger {
 public:
  ~BigInteger() = default;
  BigInteger() { blocks_.push_back(0); }
  BigInteger(const BigInteger& other) = default;
  BigInteger(const BigInteger& other, bool sign)
      : sign_(sign), blocks_(other.blocks_) {}
  BigInteger& operator=(const BigInteger& other) = default;
  BigInteger(bool sign, const std::vector<uint64_t>& blocks)
      : sign_(sign), blocks_(blocks) {}
  BigInteger(int64_t value) {
    sign_ = value >= 0;
    value = std::abs(value);
    if (value == 0) {
      blocks_.push_back(0);
      return;
    }
    while (value != 0) {
      blocks_.push_back(value % base_);
      value /= base_;
    }
  }
  BigInteger(const std::string& str) {
    if (str[0] == '-') {
      if (str[1] == '0') {
        sign_ = true;
        blocks_.push_back(0);
        return;
      }
      sign_ = false;
      blocks_.resize((str.size() + base_power_ - 2) / base_power_, 0);
    } else {
      blocks_.resize((str.size() + base_power_ - 1) / base_power_, 0);
    }
    for (size_t block = 0; block < blocks_.size(); ++block) {
      size_t j = 0;
      int current = str.size() - (block + 1) * base_power_ + j;
      while (current < 0) {
        ++current;
        ++j;
      }
      if (current == 0 && str[0] == '-') {
        ++current;
        ++j;
      }
      while (j != base_power_ && current < static_cast<int>(str.size()) &&
             current >= 0) {
        blocks_[block] = blocks_[block] * 10 + (str[current] - '0');
        ++j;
        ++current;
      }
    }
  }
  BigInteger& operator+=(const BigInteger& other) {
    if (!is_negative() && other.is_negative()) {
      return (*this -= (-other));
    }
    if (is_negative() && !other.is_negative()) {
      return (*this -= (-other));
    }
    size_t max_sz = std::max(size(), other.size());
    blocks_.resize(max_sz, 0);
    uint64_t tmp = 0;
    for (size_t i = 0; i < other.size(); ++i) {
      tmp += blocks_[i] + other.blocks_[i];
      blocks_[i] = tmp % base_;
      tmp = tmp / base_;
    }
    if (tmp != 0) {
      size_t i = other.size();
      while (tmp != 0 && i != size()) {
        tmp += blocks_[i];
        blocks_[i] = tmp % base_;
        tmp = tmp / base_;
        ++i;
      }
      if (i == size()) {
        while (tmp != 0) {
          blocks_.push_back(tmp % base_);
          tmp /= base_;
        }
      }
    }
    return *this;
  }
  BigInteger& operator-=(const BigInteger& other) {
    if (is_positive() && other.is_negative()) {
      return (*this += (-other));
    }
    if (is_negative() && other.is_positive()) {
      return (*this += (-other));
    }
    if (is_negative() && other.is_negative()) {
      return (*this = -((-*this) - (-other)));
    }
    bool sign_result = (*this >= other);
    sign_ = true;
    BigInteger max = abs_max(other);
    BigInteger min = abs_min(other);
    int64_t tmp = 0;
    for (size_t i = 0; i < min.size(); ++i) {
      tmp += static_cast<int64_t>(max.blocks_[i]) -
             static_cast<int64_t>(min.blocks_[i]);
      if (tmp >= 0) {
        max.blocks_[i] = tmp % base_;
        tmp = tmp / base_;
      } else {
        max.blocks_[i] = base_ + tmp;
        tmp = -1;
      }
    }
    if (tmp != 0) {
      size_t i = min.size();
      while (tmp != 0 && i != max.size()) {
        tmp += max.blocks_[i];
        if (tmp >= 0) {
          max.blocks_[i] = tmp % base_;
          tmp = tmp / base_;
        } else {
          max.blocks_[i] = base_ + tmp;
          tmp = -1;
        }
        ++i;
      }
      while (tmp != 0) {
        if (tmp >= 0) {
          max.blocks_.push_back(tmp % base_);
          tmp = tmp / base_;
        } else {
          max.blocks_.push_back(base_ + tmp);
          tmp = -1;
        }
      }
    }
    max.check_zeroes();
    *this = max;
    sign_ = sign_result;
    return *this;
  }
  BigInteger operator-() const {
    if (*this == 0) {
      return *this;
    }
    return BigInteger(!sign_, blocks_);
  }
  BigInteger operator+() const { return *this; }
  BigInteger& operator*=(const BigInteger& other) {
    bool sign_result;
    if ((is_negative() && other.is_negative()) ||
        (!is_negative() && !other.is_negative())) {
      sign_result = true;
    } else {
      sign_result = false;
    }
    sign_ = true;
    if (other == 0 || *this == 0) {
      *this = 0;
      return *this;
    }
    BigInteger result;
    for (size_t i = 0; i < other.size(); ++i) {
      BigInteger tmp(*this);
      tmp.short_mult(other.blocks_[i]);
      result.add_with_shift(tmp, i);
    }
    *this = result;
    sign_ = sign_result;
    return *this;
  }
  BigInteger& operator/=(const BigInteger& other) {
    bool sign_result;
    if ((is_negative() && other.is_negative()) ||
        (is_positive() && other.is_positive())) {
      sign_result = true;
    } else {
      sign_result = false;
    }
    sign_ = true;
    if (*this == 0 || abs_lower(other)) {
      *this = 0;
      return *this;
    }
    if (other == 1 || other == -1) {
      sign_ = sign_result;
      return *this;
    }
    int max_size = size() - other.size();
    BigInteger other_cp = BigInteger(other, true);
    BigInteger result = power(max_size);
    for (int i = max_size; i >= 0; --i) {
      int64_t left = 0;
      int64_t right = base_;
      while (true) {
        int64_t middle = (right + left) >> 1;
        BigInteger tmp = other_cp;
        tmp.short_mult(middle);
        tmp.shift(i);
        if (tmp > *this) {
          right = middle;
        } else {
          left = middle;
        }
        if (right - left == 1) {
          tmp = other_cp;
          tmp.short_mult(left);
          tmp.shift(i);
          *this -= tmp;
          result.blocks_[i] = left;
          break;
        }
      }
    }
    result.check_zeroes();
    *this = result;
    sign_ = sign_result;
    return *this;
  }
  BigInteger& operator++() {
    *this += 1;
    return *this;
  }
  BigInteger operator++(int) {
    BigInteger old(*this);
    *this += 1;
    return old;
  }
  BigInteger& operator--() {
    *this -= 1;
    return *this;
  }
  BigInteger operator--(int) {
    BigInteger old(*this);
    *this -= 1;
    return old;
  }
  std::string toString() const {
    std::string result;
    if (sign_ == false) {
      result.push_back('-');
    }
    result += std::to_string(*blocks_.rbegin());
    for (auto i = blocks_.rbegin() + 1; i != blocks_.rend(); ++i) {
      size_t count = count_digits(*i);
      while (count < base_power_) {
        ++count;
        result.push_back('0');
      }
      if (*i != 0) {
        result += std::to_string(*i);
      }
    }
    return result;
  }
  explicit operator bool() const { return *this != BigInteger(0); }
  explicit operator double() const {
    double ans = 0;
    for (size_t i = 0; i < blocks_.size(); ++i) {
      ans *= base_;
      ans += blocks_[blocks_.size() - 1 - i];
    }
    return ans;
  }
  bool abs_equal(const BigInteger& other) {
    if (size() != other.size()) {
      return false;
    }
    return blocks_ == other.blocks_;
  }
  BigInteger abs_max(const BigInteger& other) {
    if (size() > other.size()) {
      return BigInteger(*this, true);
    }
    if (size() < other.size()) {
      return BigInteger(other, true);
    }
    size_t i = size();
    do {
      --i;
      if (blocks_[i] < other.blocks_[i]) {
        return BigInteger(other, true);
      } else if (blocks_[i] != other.blocks_[i]) {
        return BigInteger(*this, true);
      }
    } while (i != 0);
    return BigInteger(*this, true);
  }
  BigInteger abs_min(const BigInteger& other) {
    if (size() > other.size()) {
      return BigInteger(other, true);
    }
    if (size() < other.size()) {
      return BigInteger(*this, true);
    }
    size_t i = size();
    do {
      --i;
      if (blocks_[i] < other.blocks_[i]) {
        return BigInteger(*this, true);
      } else if (blocks_[i] != other.blocks_[i]) {
        return BigInteger(other, true);
      }
    } while (i != 0);
    return BigInteger(other, true);
  }
  bool is_negative() const { return sign_ == false; }
  bool is_positive() const { return sign_ == true && *this != 0; }
  void change_sign() { sign_ ^= true; }
  void check_zeroes() {
    bool all_is_zero = false;
    int i = blocks_.size() - 1;
    while (i > 0) {
      if (blocks_[i] == 0) {
        blocks_.pop_back();
      } else {
        all_is_zero = false;
        break;
      }
      --i;
    }
    if (all_is_zero) {
      blocks_.clear();
      blocks_.push_back(0);
      sign_ = true;
    }
  }
  bool sign() const { return sign_; }
  size_t size() const { return blocks_.size(); }
  friend bool operator==(const BigInteger& first, const BigInteger& other);
  friend bool operator<(const BigInteger& first, const BigInteger& other);
  BigInteger power(int64_t power) {
    BigInteger res(0);
    res.blocks_.clear();
    if (power < 0) {
      return res;
    }
    res.blocks_.insert(res.blocks_.begin(), power, 0);
    res.blocks_.push_back(1);
    return res;
  }

 private:
  void shift(size_t shift) { blocks_.insert(blocks_.begin(), shift, 0); }
  void add_with_shift(const BigInteger& other, size_t shift) {
    BigInteger tmp(other);
    tmp.blocks_.insert(tmp.blocks_.begin(), shift, 0);
    *this += tmp;
  }
  void short_div(int64_t value) {
    if ((is_negative() && value < 0) || (!is_negative() && value >= 0)) {
      sign_ = true;
    } else {
      sign_ = false;
    }
    if (value == 0) {
      throw std::overflow_error("Divide by zero exception");
    }
    value = std::abs(value);
    BigInteger result;
    result.blocks_.resize(blocks_.size(), 0);
    uint64_t tmp = 0;
    size_t i = size();
    do {
      --i;
      int64_t cur = blocks_[i] + tmp * base_;
      result.blocks_[i] = cur / value;
      tmp = cur % value;
    } while (i != 0);
    while (result.blocks_.size() > 1 &&
           result.blocks_[result.blocks_.size() - 1] == 0) {
      result.blocks_.pop_back();
    }
    if (result.blocks_.size() == 0) {
      result.blocks_.push_back(0);
    }
    *this = result;
  }
  void short_mult(int64_t value) {
    if ((is_negative() && value < 0) || (!is_negative() && value >= 0)) {
      sign_ = true;
    } else {
      sign_ = false;
    }
    if (value == 0) {
      blocks_.clear();
      blocks_.push_back(0);
      return;
    }
    value = std::abs(value);
    uint64_t tmp = 0;
    for (size_t i = 0; i < size(); ++i) {
      tmp += blocks_[i] * value;
      blocks_[i] = tmp % base_;
      tmp = tmp / base_;
    }
    while (tmp != 0) {
      blocks_.push_back(tmp % base_);
      tmp = tmp / base_;
    }
  }
  bool abs_lower(const BigInteger& other) {
    if (size() > other.size()) {
      return false;
    }
    if (size() < other.size()) {
      return true;
    }
    if (size() == 0) {
      return false;
    }
    size_t i = size();
    do {
      --i;
      if (blocks_[i] < other.blocks_[i]) {
        return true;
      } else if (blocks_[i] != other.blocks_[i]) {
        return false;
      }
    } while (i != 0);
    return false;
  }
  static const int64_t base_ = 1000000000;
  static const size_t base_power_ = 9;
  bool sign_ = true;  // true <-> non-negative; false <-> negative
  std::vector<uint64_t> blocks_;
};

bool operator==(const BigInteger& first, const BigInteger& other) {
  if (first.size() != other.size() || first.sign() != other.sign()) {
    return false;
  }
  return first.blocks_ == other.blocks_;
}
bool operator!=(const BigInteger& first, const BigInteger& other) {
  return !(first == other);
}
bool operator<(const BigInteger& first, const BigInteger& other) {
  if (!first.is_negative() && other.is_negative()) {
    return false;
  }
  if (first.is_negative() && !other.is_negative()) {
    return true;
  }
  if (first.is_negative() && other.is_negative()) {
    return ((-other) < (-first));
  }
  if (first.size() > other.size()) {
    return false;
  }
  if (first.size() < other.size()) {
    return true;
  }
  if (first.size() == 0) {
    return false;
  }
  size_t i = first.size();
  do {
    --i;
    if (first.blocks_[i] < other.blocks_[i]) {
      return true;
    } else if (first.blocks_[i] != other.blocks_[i]) {
      return false;
    }
  } while (i != 0);
  return false;
}
bool operator>(const BigInteger& first, const BigInteger& other) {
  return other < first;
}
bool operator>=(const BigInteger& first, const BigInteger& other) {
  return !(first < other);
}
bool operator<=(const BigInteger& first, const BigInteger& other) {
  return !(other < first);
}

BigInteger operator+(const BigInteger& first, const BigInteger& second) {
  BigInteger tmp(first);
  return tmp += second;
}

BigInteger operator-(const BigInteger& first, const BigInteger& second) {
  BigInteger tmp(first);
  return tmp -= second;
}

BigInteger operator*(const BigInteger& first, const BigInteger& second) {
  BigInteger tmp(first);
  return tmp *= second;
}

BigInteger operator/(const BigInteger& first, const BigInteger& second) {
  BigInteger tmp(first);
  return tmp /= second;
}

BigInteger& operator%=(BigInteger& first, const BigInteger& second) {
  return first -= (first / second) * second;
}

BigInteger operator%(const BigInteger& first, const BigInteger& second) {
  BigInteger tmp(first);
  return tmp %= second;
}

std::ostream& operator<<(std::ostream& out, const BigInteger& value) {
  out << value.toString();
  return out;
}

std::istream& operator>>(std::istream& in, BigInteger& value) {
  std::string str;
  in >> str;
  value = BigInteger(str);
  return in;
}

BigInteger operator""_bi(unsigned long long value) { return BigInteger(value); }

BigInteger gcd(const BigInteger& first, const BigInteger& second) {
  BigInteger gcd1(first, true);
  BigInteger gcd2(second, true);
  while (gcd1 != 0 && gcd2 != 0) {
    if (gcd1 > gcd2) {
      gcd1 %= gcd2;
    } else {
      gcd2 %= gcd1;
    }
  }
  if (gcd1 != 0) {
    return gcd1;
  }
  return gcd2;
}

class Rational {
 public:
  Rational() = default;
  ~Rational() = default;
  Rational(const Rational& other) = default;
  Rational(const BigInteger& numerator, const BigInteger& denominator = 1)
      : numerator_(numerator), denominator_(denominator) {
    change_sign();
  }
  Rational(int64_t other) : numerator_(other) {}
  Rational& operator=(const Rational& other) = default;
  Rational& operator+=(const Rational& other) {
    numerator_ =
        numerator_ * other.denominator_ + denominator_ * other.numerator_;
    denominator_ *= other.denominator_;
    change_sign();
    return *this;
  }
  Rational& operator-=(const Rational& other) {
    numerator_ =
        numerator_ * other.denominator_ - denominator_ * other.numerator_;
    denominator_ *= other.denominator_;
    change_sign();
    return *this;
  }
  Rational& operator*=(const Rational& other) {
    numerator_ *= other.numerator_;
    denominator_ *= other.denominator_;
    change_sign();
    return *this;
  }
  Rational& operator/=(const Rational& other) {
    numerator_ *= other.denominator_;
    denominator_ *= other.numerator_;
    change_sign();
    return *this;
  }
  Rational operator-() const { return Rational(-numerator_, denominator_); }
  Rational operator+() const { return *this; }
  std::string toString() {
    reduce();
    if (denominator_ == 1) {
      return numerator_.toString();
    } else {
      return numerator_.toString() + '/' + denominator_.toString();
    }
  }
  std::string asDecimal(size_t precision = 0) {
    reduce();
    BigInteger tmp;
    BigInteger new_numerator;
    if (numerator_.is_negative()) {
      tmp = -numerator_ / denominator_;
      new_numerator = -numerator_ - tmp * denominator_;
    } else {
      tmp = numerator_ / denominator_;
      new_numerator = numerator_ - tmp * denominator_;
    }
    std::string result;
    if (numerator_.is_negative()) {
      result.push_back('-');
    }
    result += tmp.toString();
    if (precision == 0) {
      return result;
    }
    result.push_back('.');
    for (size_t i = 0; i + 1 < precision; ++i) {
      new_numerator *= 10;
      tmp = new_numerator / denominator_;
      if (tmp < 0) {
        tmp.change_sign();
      }
      new_numerator -= tmp * denominator_;
      result += tmp.toString();
    }
    new_numerator *= 10;
    tmp = new_numerator / denominator_;
    new_numerator -= tmp * denominator_;
    BigInteger next_tmp = new_numerator / denominator_;
    if (next_tmp < 0) {
      next_tmp.change_sign();
    }
    if (next_tmp >= 5) {
      ++tmp;
    }
    result += tmp.toString();
    return result;
  }
  explicit operator double() const {
    return static_cast<double>(numerator_) / static_cast<double>(denominator_);
  }
  friend bool operator==(const Rational& first, const Rational& other);
  friend bool operator<(const Rational& first, const Rational& other);

 private:
  BigInteger numerator_ = 0;
  BigInteger denominator_ = 1;
  void change_sign() {
    if (denominator_.is_negative()) {
      denominator_.change_sign();
      numerator_.change_sign();
    }
  }
  void reduce() {
    change_sign();
    if (numerator_ == 0) {
      denominator_ = 1;
      return;
    }
    if (denominator_ == 1) {
      return;
    }
    BigInteger gcd_result = gcd(numerator_, denominator_);
    numerator_ /= gcd_result;
    denominator_ /= gcd_result;
    change_sign();
  }
};

bool operator==(const Rational& first, const Rational& other) {
  return first.numerator_ * other.denominator_ ==
         other.numerator_ * first.denominator_;
}
bool operator!=(const Rational& first, const Rational& other) {
  return !(first == other);
}
bool operator<(const Rational& first, const Rational& other) {
  if (first.numerator_.is_negative() && !other.numerator_.is_negative()) {
    return true;
  }
  if (!first.numerator_.is_negative() && other.numerator_.is_negative()) {
    return false;
  }
  return first.numerator_ * other.denominator_ <
         other.numerator_ * first.denominator_;
}
bool operator>(const Rational& first, const Rational& other) {
  return other < first;
}
bool operator<=(const Rational& first, const Rational& other) {
  return !(other < first);
}
bool operator>=(const Rational& first, const Rational& other) {
  return !(other > first);
}

Rational operator+(const Rational& first, const Rational& second) {
  Rational tmp(first);
  return tmp += second;
}
Rational operator-(const Rational& first, const Rational& second) {
  Rational tmp(first);
  return tmp -= second;
}
Rational operator*(const Rational& first, const Rational& second) {
  Rational tmp(first);
  return tmp *= second;
}
Rational operator/(const Rational& first, const Rational& second) {
  Rational tmp(first);
  return tmp /= second;
}

