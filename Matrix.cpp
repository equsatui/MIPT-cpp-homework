#include <algorithm>
#include <array>
#include <cassert>
#include <iomanip>
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
  std::string asи(size_t precision = 0) const {
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
    auto result = asDecimal(20);
    return std::stod(result);
  }
  friend bool operator==(const Rational& first, const Rational& other);
  friend bool operator<(const Rational& first, const Rational& other);
  friend Rational inversed_element(const Rational& value);
  friend std::istream& operator>>(std::istream& in, Rational& value);

 private:
  BigInteger numerator_ = 0;
  BigInteger denominator_ = 1;
  void change_sign() {
    reduce();
    if (denominator_.is_negative()) {
      denominator_.change_sign();
      numerator_.change_sign();
    }
  }
  void reduce() {
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

std::istream& operator>>(std::istream& in, Rational& value) {
  std::string str;
  in >> str;
  std::string num;
  std::string denom;
  size_t index = 0;
  while (index < str.size() && str[index] != '/') {
    num += str[index];
    ++index;
  }
  while (index < str.size()) {
    denom += str[index];
    ++index;
  }
  if (denom.size() == 0) {
    denom = "1";
  }
  value.numerator_ = BigInteger(num);
  value.denominator_ = BigInteger(denom);
  value.change_sign();
  return in;
}

template <size_t N, size_t mod, bool Stop = ((mod * mod) > N)>
struct prime_recursion {
  static const bool value =
      (N % mod != 0) && (Stop ? true : prime_recursion<N, mod + 1>::value);
};

template <size_t N, size_t mod>
struct prime_recursion<N, mod, true> {
  static const bool value = true;
};

template <size_t N, bool flag>
struct prime_recursion<N, 1, flag> {
  static const bool value = flag;
};

template <size_t N>
struct prime_check {
  static const bool value = prime_recursion<N, 2>::value;
};

template <>
struct prime_check<1> {
  static const bool value = false;
};

template <size_t N, bool flag = prime_check<N>::value>
struct Residue {
 public:
  int64_t value_;
  Residue(int64_t value = 0) {
    if (value >= 0) {
      value_ = value % N;
    } else {
      value_ = (static_cast<int64_t>(N) + value % static_cast<int64_t>(N)) %
               static_cast<int64_t>(N);
    }
  }
  int64_t& operator--() {
    if (value_ == 0) {
      value_ = N - 1;
    } else {
      --value_;
    }
    return value_;
  }
  explicit operator int() const { return static_cast<int>(value_); }
  void Reduce() {
    if (value_ >= 0) {
      value_ = value_ % N;
    } else {
      value_ = (static_cast<int64_t>(N) + value_ % static_cast<int64_t>(N)) %
               static_cast<int64_t>(N);
    }
  }
};

template <size_t N>
bool operator==(const Residue<N>& first, const Residue<N>& second) {
  return first.value_ == second.value_;
}
template <size_t N>
bool operator!=(const Residue<N>& first, const Residue<N>& second) {
  return first.value_ != second.value_;
}
template <size_t N>
bool operator==(const Residue<N>& first, int second) {
  return first.value_ == second;
}
template <size_t N>
bool operator<(const Residue<N>& first, const Residue<N>& second) {
  return first.value_ < second.value_;
}
template <size_t N>
Residue<N> operator+(const Residue<N>& first, const Residue<N>& second) {
  return Residue<N>(first.value_ + second.value_);
}
template <size_t N>
Residue<N>& operator+=(Residue<N>& first, const Residue<N>& second) {
  first.value_ += second.value_;
  first.Reduce();
  return first;
}
template <size_t N>
Residue<N> operator-(const Residue<N>& first, const Residue<N>& second) {
  return Residue<N>(first.value_ - second.value_);
}
template <size_t N>
Residue<N>& operator-=(Residue<N>& first, const Residue<N>& second) {
  first.value_ -= second.value_;
  first.Reduce();
  return first;
}
template <size_t N>
Residue<N> operator*(const Residue<N>& first, const Residue<N>& second) {
  return Residue<N>(first.value_ * second.value_);
}
template <size_t N>
Residue<N>& operator*=(Residue<N>& first, const Residue<N>& second) {
  first.value_ *= second.value_;
  first.Reduce();
  return first;
}

template <size_t N>
Residue<N, true> binary_power(Residue<N, true> value, size_t power) {
  int64_t result = 1;
  int64_t val = value.value_;
  while (power != 0) {
    if (power % 2 == 1) {
      result = (result * val) % N;
    }
    val = (val * val) % N;
    power /= 2;
  }
  return Residue<N, true>(result);
}
template <size_t N>
Residue<N, true> inversed_element(const Residue<N, true>& value) {
  return binary_power(value, N - 2);
}
Rational inversed_element(const Rational& value) {
  return Rational(value.denominator_, value.numerator_);
}
long double inversed_element(long double value) { return 1 / (value); }
double inversed_element(double value) { return 1 / (value); }
template <size_t N>
Residue<N, true> operator/(const Residue<N, true>& first,
                           const Residue<N, true>& second) {
  return first * inversed_element(second);
}

template <size_t N>
Residue<N, true> abs(const Residue<N, true>& first) {
  return first;
}
Rational abs(const Rational& first) {
  return first * (first < Rational(0) ? Rational(-1) : Rational(1));
}

template <size_t M, size_t N, typename Field>
struct Matrix;

template <size_t A, size_t B, typename Field>
void out(Matrix<A, B, Field> matrix) {
  for (size_t row = 0; row < A; ++row) {
    for (auto& el : matrix.values_[row]) {
      std::cout << std::setw(15) << el.value_ << ' ';
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

template <size_t M, size_t N, typename Field = Rational>
struct Matrix {
  std::array<std::array<Field, N>, M> values_;
  Matrix(const std::initializer_list<std::initializer_list<Field>>& list) {
    size_t i = 0;
    size_t j = 0;
    for (auto& row : list) {
      for (auto& element : row) {
        values_[i][j] = element;
        ++j;
      }
      ++i;
      j = 0;
    }
  }
  Matrix(const Matrix<M, N, Field>& other) {
    for (size_t i = 0; i < M; ++i) {
      for (size_t j = 0; j < N; ++j) {
        values_[i][j] = other.values_[i][j];
      }
    }
  }
  Matrix<M, N, Field>& operator=(const Matrix<M, N, Field>& other) {
    for (size_t i = 0; i < M; ++i) {
      for (size_t j = 0; j < N; ++j) {
        values_[i][j] = other.values_[i][j];
      }
    }
    return *this;
  }
  Matrix(const std::array<std::array<Field, N>, M>& values) {
    for (size_t i = 0; i < M; ++i) {
      for (size_t j = 0; j < N; ++j) {
        values_[i][j] = values[i][j];
      }
    }
  };
  Matrix() {
    for (size_t i = 0; i < M; ++i) {
      for (size_t j = 0; j < N; ++j) {
        values_[i][j] = Field(0);
      }
    }
  }
  Matrix<M, N, Field>& operator=(
      const std::initializer_list<std::initializer_list<Field>>& list) {
    size_t i = 0;
    size_t j = 0;
    for (auto& row : list) {
      for (auto& element : row) {
        values_[i][j] = element;
        ++j;
      }
      ++i;
      j = 0;
    }
    return *this;
  }
  Matrix<N, M, Field> transposed() const {
    Matrix<N, M, Field> result;
    for (size_t i = 0; i < M; ++i) {
      for (size_t j = 0; j < N; ++j) {
        result.values_[j][i] = values_[i][j];
      }
    }
    return result;
  }
  std::array<Field, N> getRow(size_t row) const { return values_[row]; }
  std::array<Field, M> getColumn(size_t column) const {
    std::array<Field, M> res;
    for (size_t row_index = 0; row_index < M; ++row_index) {
      res[row_index] = values_[row_index][column];
    }
    return res;
  }
  const std::array<Field, N>& operator[](const size_t i) const {
    return values_[i];
  }
  std::array<Field, N>& operator[](const size_t i) { return values_[i]; }
  size_t rank() const {
    Matrix<M, N, Field> copy(values_);
    size_t row = 0;
    size_t column = 0;
    while (row < M && column < N) {
      Field max(copy.values_[row][column]);
      size_t max_index = row;
      do {
        for (size_t cur_row = row + 1; cur_row < M; ++cur_row) {
          if (abs(max) < abs(copy.values_[cur_row][column])) {
            max_index = cur_row;
            max = copy.values_[cur_row][column];
          }
        }
        if (max == 0) {
          ++column;
          if (column == N) {
            break;
          }
          max = copy.values_[row][column];
        }
      } while (max == 0);
      if (column == N) {
        break;
      }
      std::swap(copy.values_[row], copy.values_[max_index]);
      Field inv = 0;
      for (size_t i = row + 1; i < M; ++i) {
        inv = copy.values_[i][column] / copy.values_[row][column];
        for (size_t j = column; j < N; ++j) {
          copy.values_[i][j] -= inv * copy.values_[row][j];
        }
      }
      ++row;
    }
    size_t rank = 0;
    for (size_t i = 0; i < M; ++i) {
      bool empty = true;
      for (size_t j = 0; j < N; ++j) {
        if (copy.values_[i][j] != 0) {
          empty = false;
          break;
        }
      }
      if (!empty) {
        ++rank;
      }
    }
    return rank;
  }
};

template <size_t N, typename Field>
struct Matrix<N, N, Field> {
  std::array<std::array<Field, N>, N> values_;
  Matrix(const std::initializer_list<std::initializer_list<Field>>& list) {
    size_t i = 0;
    size_t j = 0;
    for (auto& row : list) {
      for (auto& element : row) {
        values_[i][j] = element;
        ++j;
      }
      ++i;
      j = 0;
    }
  }
  Matrix(const std::array<std::array<Field, N>, N>& values) {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        values_[i][j] = values[i][j];
      }
    }
  }
  Matrix() {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        values_[i][j] = Field(0);
      }
    }
  }
  Matrix(const Matrix<N, N, Field>& other) {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        values_[i][j] = other.values_[i][j];
      }
    }
  }
  Matrix<N, N, Field>& operator=(const Matrix<N, N, Field>& other) {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        values_[i][j] = other.values_[i][j];
      }
    }
    return *this;
  }
  Matrix<N, N, Field>& operator=(
      const std::initializer_list<std::initializer_list<Field>>& list) {
    size_t i = 0;
    size_t j = 0;
    for (auto& row : list) {
      for (auto& element : row) {
        values_[i][j] = element;
        ++j;
      }
      ++i;
      j = 0;
    }
    return *this;
  }
  Matrix<N, N, Field> transposed() const {
    Matrix<N, N, Field> result;
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        result.values_[j][i] = values_[i][j];
      }
    }
    return result;
  }
  std::array<Field, N> getRow(size_t row) const { return values_[row]; }
  std::array<Field, N> getColumn(size_t column) const {
    std::array<Field, N> res;
    for (size_t row_index = 0; row_index < N; ++row_index) {
      res[row_index] = values_[row_index][column];
    }
    return res;
  }
  const std::array<Field, N>& operator[](const size_t i) const {
    return values_[i];
  }
  std::array<Field, N>& operator[](const size_t i) { return values_[i]; }
  Field trace() const {
    Field result(0);
    for (size_t i = 0; i < N; ++i) {
      result += values_[i][i];
    }
    return result;
  }
  Field det() const {
    Field determinant(1);
    Matrix<N, N, Field> copy(values_);
    for (size_t row = 0; row < N; ++row) {
      Field max(copy.values_[row][row]);
      size_t max_index = row;
      for (size_t cur_row = row + 1; cur_row < N; ++cur_row) {
        if (abs(max) < abs(copy.values_[cur_row][row])) {
          max_index = cur_row;
          max = copy.values_[cur_row][row];
        }
      }
      if (max == Field(0)) {
        return 0;
      }
      if (row != max_index) {
        determinant *= Field(-1);
        std::swap(copy.values_[row], copy.values_[max_index]);
      }
      determinant *= copy.values_[row][row];
      Field inv = 0;
      for (size_t i = row + 1; i < N; ++i) {
        inv = copy.values_[i][row] / copy.values_[row][row];
        for (size_t j = row; j < N; ++j) {
          copy.values_[i][j] -= inv * copy.values_[row][j];
        }
      }
    }
    return determinant;
  }
  Matrix<N, N, Field> inverted() const {
    Matrix<N, 2 * N, Field> bigMatrix;
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        bigMatrix.values_[i][j] = values_[i][j];
      }
    }
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        bigMatrix.values_[i][N + j] = 0;
      }
      bigMatrix.values_[i][N + i] = 1;
    }
    for (size_t row = 0; row < N; ++row) {
      Field max(bigMatrix.values_[row][row]);
      size_t max_index = row;
      for (size_t cur_row = row + 1; cur_row < N; ++cur_row) {
        if (abs(max) < abs(bigMatrix.values_[cur_row][row])) {
          max_index = cur_row;
          max = bigMatrix.values_[cur_row][row];
        }
      }
      std::swap(bigMatrix.values_[row], bigMatrix.values_[max_index]);
      for (size_t i = row + 1; i < N; ++i) {
        Field inv = bigMatrix.values_[i][row] / bigMatrix.values_[row][row];
        for (size_t j = row; j < 2 * N; ++j) {
          bigMatrix.values_[i][j] -= inv * bigMatrix.values_[row][j];
        }
      }
      for (size_t i = 0; i < row; ++i) {
        Field inv = bigMatrix.values_[i][row] / bigMatrix.values_[row][row];
        for (size_t j = row; j < N * 2; ++j) {
          bigMatrix.values_[i][j] -= inv * bigMatrix.values_[row][j];
        }
      }
    }
    Matrix<N, N, Field> Result;
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        Result.values_[i][j] =
            bigMatrix.values_[i][N + j] / bigMatrix.values_[i][i];
      }
    }
    return Result;
  }
  Matrix<N, N, Field>& invert() {
    Matrix<N, N, Field> result = inverted();
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        values_[i][j] = result.values_[i][j];
      }
    }
    return *this;
  }
  size_t rank() const {
    Matrix<N, N, Field> copy(values_);
    size_t row = 0;
    size_t column = 0;
    while (row < N && column < N) {
      Field max(copy.values_[row][column]);
      size_t max_index = row;
      do {
        for (size_t cur_row = row + 1; cur_row < N; ++cur_row) {
          if (max < copy.values_[cur_row][column]) {
            max_index = cur_row;
            max = copy.values_[cur_row][column];
          }
        }
        if (max == 0) {
          ++column;
          if (column == N) {
            break;
          }
          max = copy.values_[row][column];
        }
      } while (max == 0);
      if (column == N) {
        break;
      }
      std::swap(copy.values_[row], copy.values_[max_index]);
      Field inv = 0;
      for (size_t i = row + 1; i < N; ++i) {
        inv = copy.values_[i][column] / copy.values_[row][column];
        for (size_t j = column; j < N; ++j) {
          copy.values_[i][j] -= inv * copy.values_[row][j];
        }
      }
      ++row;
    }
    size_t rank = 0;
    for (size_t i = 0; i < N; ++i) {
      bool empty = true;
      for (size_t j = 0; j < N; ++j) {
        if (copy.values_[i][j] != Field(0)) {
          empty = false;
          break;
        }
      }
      if (!empty) {
        ++rank;
      }
    }
    return rank;
  }
};

template <size_t N, typename Field = Rational>
using SquareMatrix = Matrix<N, N, Field>;

template <size_t M, size_t N, typename Field>
Matrix<M, N, Field>& operator+=(Matrix<M, N, Field>& first,
                                const Matrix<M, N, Field>& other) {
  for (size_t i = 0; i < M; ++i) {
    for (size_t j = 0; j < N; ++j) {
      first.values_[i][j] += other.values_[i][j];
    }
  }
  return first;
}

template <size_t M, size_t N, typename Field>
Matrix<M, N, Field>& operator-=(Matrix<M, N, Field>& first,
                                const Matrix<M, N, Field>& other) {
  for (size_t i = 0; i < M; ++i) {
    for (size_t j = 0; j < N; ++j) {
      first.values_[i][j] -= other.values_[i][j];
    }
  }
  return first;
}

template <size_t M, size_t N, typename Field>
Matrix<M, N, Field> operator+(const Matrix<M, N, Field>& first,
                              const Matrix<M, N, Field>& other) {
  auto cp(first);
  return cp += other;
}

template <size_t M, size_t N, typename Field>
Matrix<M, N, Field> operator-(const Matrix<M, N, Field>& first,
                              const Matrix<M, N, Field>& other) {
  auto cp(first);
  return cp -= other;
}

template <size_t M, size_t N, typename Field>
Matrix<M, N, Field>& operator*=(Matrix<M, N, Field>& matrix,
                                const Field& value) {
  for (size_t i = 0; i < M; ++i) {
    for (size_t j = 0; j < N; ++j) {
      matrix.values_[i][j] *= value;
    }
  }
  return matrix;
}

template <size_t M, size_t N, typename Field>
Matrix<M, N, Field> operator*(const Matrix<M, N, Field>& matrix,
                              const Field& value) {
  auto cp(matrix);
  return cp *= value;
}

template <size_t M, size_t N, typename Field>
Matrix<M, N, Field> operator*(const Field& value,
                              const Matrix<M, N, Field>& matrix) {
  auto cp(matrix);
  return cp *= value;
}

template <size_t M, size_t N, size_t K, typename Field>
Matrix<M, K, Field> operator*(const Matrix<M, N, Field>& first,
                              const Matrix<N, K, Field>& second) {
  Matrix<M, K, Field> result;
  for (size_t row = 0; row < M; ++row) {
    for (size_t column = 0; column < K; ++column) {
      result.values_[row][column] = Field(0);
      for (size_t i = 0; i < N; ++i) {
        result.values_[row][column] +=
            first.values_[row][i] * second.values_[i][column];
      }
    }
  }
  return result;
}

template <size_t N, typename Field>
Matrix<N, N, Field>& operator*=(Matrix<N, N, Field>& first,
                                const Matrix<N, N, Field>& second) {
  Matrix<N, N, Field> result;
  for (size_t row = 0; row < N; ++row) {
    for (size_t column = 0; column < N; ++column) {
      result.values_[row][column] = Field(0);
      for (size_t i = 0; i < N; ++i) {
        result.values_[row][column] +=
            first.values_[row][i] * second.values_[i][column];
      }
    }
  }
  first = result;
  return first;
}

template <typename A, typename B>
struct is_same {
  static const bool value = false;
};

template <typename A>
struct is_same<A, A> {
  static const bool value = true;
};

template <size_t A, size_t B, size_t C, size_t D, typename Field_1,
          typename Field_2>
bool operator==(const Matrix<A, B, Field_1>& first,
                const Matrix<C, D, Field_2>& second) {
  if (A != C || B != D || !is_same<Field_1, Field_2>::value) {
    return false;
  }
  for (size_t i = 0; i < A; ++i) {
    for (size_t j = 0; j < B; ++j) {
      if (first.values_[i][j] != second.values_[i][j]) {
        return false;
      }
    }
  }
  return true;
}

