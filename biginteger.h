#include <iostream>
#include <algorithm>
#include <string>
#include <vector>

class BigInteger {
  private:
  static const int BASE = 1e4;
  static const int BASE_LENGTH = 4;
  std::vector<long long> data_;
  bool isPositive = true;
  void swap(BigInteger& other);

  public:
  BigInteger();
  BigInteger(long long number);
  BigInteger(const std::string &str_number);
  std::string toString() const;
  BigInteger& operator=(BigInteger num);

  BigInteger& operator+=(const BigInteger& other);
  BigInteger& operator-=(const BigInteger& other);
  BigInteger& operator*=(const BigInteger& other);
  BigInteger& operator/=(const BigInteger& other);
  BigInteger& operator%=(const BigInteger& other);

  BigInteger& operator++();
  BigInteger operator++(int);
  BigInteger& operator--();
  BigInteger operator--(int);
  BigInteger operator-() const;

  explicit operator bool() const;

  friend std::ostream &operator<<(std::ostream &out, const BigInteger& number);
  friend std::istream &operator>>(std::istream &in, BigInteger& number);
  friend bool operator<(const BigInteger& first, const BigInteger& second);

  bool sign_number() const {
    return isPositive;
  }
  size_t size() const {
    return data_.size();
  }

  ~BigInteger() = default;
};

BigInteger operator+(const BigInteger& first, const BigInteger& second);
BigInteger operator-(const BigInteger& first, const BigInteger& second);
BigInteger operator*(const BigInteger& first, const BigInteger& second);
BigInteger operator/(const BigInteger& first, const BigInteger& second);
BigInteger operator%(const BigInteger& first, const BigInteger& second);

bool operator>(const BigInteger& first, const BigInteger& second);
bool operator<=(const BigInteger& first, const BigInteger& second);
bool operator>=(const BigInteger& first, const BigInteger& second);
bool operator==(const BigInteger& first, const BigInteger& second);
bool operator!=(const BigInteger& first, const BigInteger& second);

class Rational {
  private:
  BigInteger numerator;
  BigInteger denominator;
  BigInteger gcd(const BigInteger& first, const BigInteger& second) const;
  void change_fraction();
  static const int BASE = 1e4;
  static const int BASE_LENGTH = 4;
  static const unsigned int DECIMAL_BASE = 16;
  void result_rational_sign();

  public:
  Rational();
  Rational(long long number);
  Rational(const BigInteger& number);
  std::string toString() const;
  std::string asDecimal(size_t precision) const;

  Rational& operator+=(const Rational& other);
  Rational& operator-=(const Rational& other);
  Rational& operator*=(const Rational& other);
  Rational& operator/=(const Rational& other);

  friend bool operator<(const Rational& first, const Rational& second);
  explicit operator double() const;

  Rational operator-() const;
};

Rational operator+(const Rational& first, const Rational& second);
Rational operator-(const Rational& first, const Rational& second);
Rational operator*(const Rational& first, const Rational& second);
Rational operator/(const Rational& first, const Rational& second);
Rational operator%(const Rational& first, const Rational& second);
bool operator>(const Rational& first, const Rational& second);
bool operator<=(const Rational& first, const Rational& second);
bool operator>=(const Rational& first, const Rational& second);
bool operator==(const Rational& first, const Rational& second);
bool operator!=(const Rational& first, const Rational& second);

BigInteger::BigInteger() : data_(1, 0), isPositive(true) {}

BigInteger operator ""_bi(const char *num) {
  return BigInteger(std::string(num));
}

BigInteger::BigInteger(long long number) {
  if (number < 0) {
    isPositive = false;
    number *= -1;
  }
  if (number == 0) {
    data_ = {0};
    return;
  }
  while (number > 0) {
    data_.push_back(number % BASE);
    number /= BASE;
  }
}

BigInteger::BigInteger(const std::string &str_number) {
  std::string number;
  if (str_number[0] == '-') {
    isPositive = false;
    number = str_number.substr(1);
  } else {
    number = str_number;
  }
  size_t number_end = number.length();
  while (number_end > BASE_LENGTH) {
    size_t number_begin = number_end - BASE_LENGTH;
    std::string data = number.substr(number_begin, BASE_LENGTH);
    data_.push_back(stoi(data));
    number_end -= BASE_LENGTH;
  }
  std::string data = number.substr(0, number_end);
  data_.push_back(stoi(data));
}

std::string BigInteger::toString() const {
  std::string result = "";
  if (!isPositive) {
    result += "-";
  }
  for (auto it = data_.rbegin(); it != data_.rend(); ++it) {
    if (it == data_.rbegin()) {
      result += std::to_string(*it);
    } else {
      for (unsigned int i = 0; i < BASE_LENGTH - std::to_string(*it).length(); ++i) {
        result += "0";
      }
      result += std::to_string(*it);
    }
  }
  return result;
}

BigInteger& BigInteger::operator=(BigInteger num) {
  swap(num);
  return *this;
}

void BigInteger::swap(BigInteger &other) {
  std::swap(data_, other.data_);
  std::swap(isPositive, other.isPositive);
}

BigInteger& BigInteger::operator+=(const BigInteger& other) {
  if (isPositive != other.isPositive) {
    if (!isPositive) {
      return *this = other - (-*this);
    }
    return *this -= (-other);
  }
  int carry = 0;
  for (size_t i = 0; i < std::max(size(), other.size()) || carry; ++i) {
    if (i == size()) {
      data_.push_back(0);
    }
    data_[i] += carry + (i < other.size() ? other.data_[i] : 0);
    carry = data_[i] >= BASE;
    if (carry) {
      data_[i] -= BASE;
    }
  }
  return *this;
}

BigInteger& BigInteger::operator-=(const BigInteger& other) {
  if (!other.isPositive) {
    return *this += (-other);
  }
  if (*this < other) {
    *this = other - *this;
    isPositive = false;
    return *this;
  }
  int carry = 0;
  for (size_t i = 0; i < other.size() || carry; ++i) {
    data_[i] -= carry + (i < other.size() ? other.data_[i] : 0);
    carry = data_[i] < 0;
    if (carry)  data_[i] += BASE;
  }
  while (size() > 1 && data_.back() == 0)
    data_.pop_back();

  if (data_.back() == 0) isPositive = true;
  return *this;
}

BigInteger& BigInteger::operator*=(const BigInteger& other) {
  if (!*this || !other) {
    *this = 0;
    isPositive = true;
    return *this;
  }
  bool result_sign = false;
  if (isPositive == other.sign_number()) {
    result_sign = true;
  }
  BigInteger result;
  result.data_.resize(size() + other.size());
  for (size_t i = 0; i < size(); ++i) {
    long long carry = 0;
    for (size_t j = 0; j < other.size() || carry; ++j) {
      long long res = result.data_[i + j] + carry;
      res += (j < other.size() ? data_[i] * other.data_[j] : 0);
      carry = res / BASE;
      result.data_[i + j] = res % BASE;
    }
  }
  while (result.size() > 1 && !result.data_.back()) {
    result.data_.pop_back();
  }
  *this = result;
  isPositive = result_sign;
  return *this;
}

BigInteger& BigInteger::operator/=(const BigInteger& other) {
  if (!other.isPositive) {
    *this /= -other;
    isPositive = false;
    return *this;
  }
  if (!isPositive) {
    *this = -*this / other;
    if (*this) {
      isPositive = false;
    }
    return *this;
  }

  if (*this < other) {
    return *this = 0;
  }
  BigInteger result;
  BigInteger x;
  result.isPositive = isPositive == other.isPositive;
  for (int i = static_cast<int>(size()) - 1; i >= 0; --i) {
    x *= BASE;
    x += data_[i];
    if (x >= other) {
      long long left = 0;
      long long right = BASE;
      while (left < right - 1) {
        long long mid = (right + left) / 2;
        if (mid * other <= x) {
          left = mid;
        } else {
          right = mid;
        }
      }
      result *= BASE;
      result += left;
      x -= left * other;
    } else {
      result *= BASE;
    }
  }
  return *this = result;
}

BigInteger& BigInteger::operator%=(const BigInteger& other) {
  BigInteger new_other = other;
  new_other.isPositive = true;
  return *this -= ((*this / new_other) * new_other);
}

BigInteger operator+(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result += second;
  return result;
}

BigInteger operator-(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result -= second;
  return result;
}
BigInteger operator*(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result *= second;
  return result;
}

BigInteger operator/(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result /= second;
  return result;
}

BigInteger operator%(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result %= second;
  return result;
}

BigInteger& BigInteger::operator++() {
  return *this += 1;
}

BigInteger& BigInteger::operator--() {
  return *this -= 1;
}

BigInteger BigInteger::operator++(int) {
  BigInteger copy = *this;
  ++*this;
  return copy;
}

BigInteger BigInteger::operator--(int) {
  BigInteger copy = *this;
  --*this;
  return copy;
}

BigInteger BigInteger::operator-() const {
  BigInteger copy = *this;
  if (copy) {
    copy.isPositive = !copy.isPositive;
  }
  return copy;
}

bool operator<(const BigInteger& first, const BigInteger& second) {
  if (first.sign_number() != second.sign_number()) {
    return !first.sign_number();
  }
  bool same_sign = first.sign_number();
  if (first.size() != second.size()) {
    return (!same_sign) ^ (first.size() < second.size());
  }
  for (long long i = static_cast<long long>(first.size()) - 1; i >= 0; --i) {
    if (first.data_[i] != second.data_[i]) {
      return (!same_sign) ^ (first.data_[i] < second.data_[i]);
    }
  }
  return false;
}

bool operator>(const BigInteger& first, const BigInteger& second) {
  return second < first;
}

bool operator<=(const BigInteger& first, const BigInteger& second) {
  return !(first > second);
}

bool operator>=(const BigInteger& first, const BigInteger& second) {
  return !(first < second);
}

bool operator==(const BigInteger& first, const BigInteger& second) {
  return !((first < second) || (first > second));
}

bool operator!=(const BigInteger& first, const BigInteger& second)  {
  return !(first == second);
}

std::ostream &operator<<(std::ostream &out, const BigInteger& number) {
  out << number.toString();
  return out;
}

std::istream &operator>>(std::istream &in, BigInteger& number) {
  std::string str;
  in >> str;
  number = BigInteger(str);
  return in;
}

BigInteger::operator bool() const {
  return *this != 0;
}

//RATIONAL

Rational::Rational() : numerator(0), denominator(1) {}

Rational::Rational(long long number) : numerator(number), denominator(1) {}

Rational::Rational(const BigInteger& number) : numerator(number), denominator(1) {}

std::string Rational::toString() const {
  std::string result = numerator.toString();
  if (denominator != 1) {
    result += "/";
    result += denominator.toString();
  }
  return result;
}

BigInteger Rational::gcd(const BigInteger& first, const BigInteger& second) const {
  if (second == 0) {
    return first;
  }
  return gcd(second, first % second);
}

void Rational::change_fraction() {
  if (numerator == 0) {
    denominator = 1;
  } else {
    BigInteger multiply;
    if (numerator >= 0) {
      multiply = gcd(numerator, denominator);
    } else {
      multiply = gcd(-numerator, denominator);
    }
    numerator /= multiply;
    denominator /= multiply;
  }
}

void Rational::result_rational_sign() {
  if (denominator < 0) {
    denominator = -denominator;
    numerator = -numerator;
  }
}

Rational& Rational::operator+=(const Rational &other) {
  numerator = numerator * other.denominator + denominator * other.numerator;
  denominator *= other.denominator;
  result_rational_sign();
  change_fraction();
  return *this;
}

Rational& Rational::operator-=(const Rational &other) {
  numerator = numerator * other.denominator - denominator * other.numerator;
  denominator *= other.denominator;
  result_rational_sign();
  change_fraction();
  return *this;
}

Rational& Rational::operator*=(const Rational &other) {
  numerator *= other.numerator;
  denominator *= other.denominator;
  result_rational_sign();
  change_fraction();
  return *this;
}

Rational& Rational::operator/=(const Rational &other) {
  numerator *= other.denominator;
  denominator *= other.numerator;
  result_rational_sign();
  change_fraction();
  return *this;
}

Rational operator+(const Rational& first, const Rational& second) {
  Rational result = first;
  result += second;
  return result;
}

Rational operator-(const Rational& first, const Rational& second) {
  Rational result = first;
  result -= second;
  return result;
}

Rational operator*(const Rational& first, const Rational& second) {
  Rational result = first;
  result *= second;
  return result;
}

Rational operator/(const Rational& first, const Rational& second) {
  Rational result = first;
  result /= second;
  return result;
}

bool operator<(const Rational& first, const Rational& second) {
  return (first.numerator * second.denominator < first.denominator * second.numerator);
}

bool operator>(const Rational& first, const Rational& second) {
  return second < first;
}

bool operator<=(const Rational& first, const Rational& second) {
  return !(first > second);
}

bool operator>=(const Rational& first, const Rational& second) {
  return !(first <= second);
}

bool operator==(const Rational& first, const Rational& second) {
  return !((first < second) || (first > second));
}
bool operator!=(const Rational& first, const Rational& second) {
  return !(first == second);
}

Rational Rational::operator-() const {
  Rational copy = *this;
  copy.numerator = -copy.numerator;
  return copy;
}

std::string Rational::asDecimal(size_t precision = 0) const {
  BigInteger num = numerator;
  BigInteger den = denominator;
  std::string first_part = "";
  std::string second_part = "";
  if (num < 0) {
    num = -num;
    first_part += "-";
  }
  first_part += (num / den).toString();
  while (second_part.length() < precision) {
    num %= den;
    num *= BASE;
    std::string add = (num / den).toString();
    while (add.length() < BASE_LENGTH) {
      add = "0" + add;
    }
    second_part += add;
  }
  while (second_part.length() > precision) {
    second_part.pop_back();
  }
  if (precision == 0) {
    return first_part;
  }
  return first_part + "." + second_part;

}

Rational::operator double() const {
  return stod(asDecimal(DECIMAL_BASE));
}
