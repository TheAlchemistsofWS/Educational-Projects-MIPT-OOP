#include <algorithm>
#include <iostream>
#include <cstring>

class String {
  private:
  int size_ = 0;
  int capacity_ = 0;
  char* data_ = nullptr;

  void changeCap();
  void swap(String &str);

  public:
  String(const char* chr) : size_(strlen(chr)), capacity_(size_), data_(new char[capacity_ + 1]) {
    std::copy(chr, chr + size_, data_);
    data_[size_] = '\0';
  }

  String(int coun, const char chr): size_(coun), capacity_(coun), data_(new char[capacity_ + 1]) {
    std::fill(data_, data_ + size_, chr);
    data_[size_] = '\0';
  }

  String() : size_(0), capacity_(0), data_(new char[1]) {
    data_[0] = '\0';
  };

  String(const String& str) : String(str.size_, '\0') {
    std::copy(str.data_, str.data_ + size_ + 1, data_);
  }

  String& operator=(String str);
  String& operator+=(const String& str);
  String& operator+=(const char& chr);

  String substr(int index, int count_index) const;

  int length() const { return size_; }

  int capacity() const { return capacity_; }

  int size() const { return size_; }

  char& operator[](int index) { return data_[index]; }

  const char& operator[](int index) const { return data_[index]; }

  const char& front() const{ return data_[0]; }

  char& front() { return data_[0]; }

  const char& back() const { return data_[size_ - 1]; }

  char& back() { return data_[size_ - 1]; }

  char* data() { return data_; }

  const char* data() const { return data_; }

  bool empty() { return size_ == 0; }

  void clear() {
    size_ = 0;
    data_[size_] = '\0';
  }

  void push_back(const char& chr) { *this += chr; }

  void pop_back() {
    data_[size_ - 1] = '\0';
    --size_;
  }

  void shrink_to_fit();

  size_t find(const String& substr_) const;

  int rfind(const String& substr_) const;

  ~String() { delete[] data_; }
};

void String::swap(String &str) {
  std::swap(str.data_, data_);
  std::swap(str.size_, size_);
  std::swap(str.capacity_, capacity_);
}

void String::changeCap() {
  if (size_ != 0) {
    capacity_ = 2 * size_;
  } else {
    capacity_ = 1;
  }
  char* tmp_data_ = new char[capacity_ + 1];
  strcpy(tmp_data_, data_);
  delete[] data_;
  data_ = tmp_data_;
}

bool operator>(const String& str_1, const String& str_2);
bool operator<=(const String& str_1, const String& str_2);
bool operator>=(const String& str_1, const String& str_2);
bool operator==(const String& str_1, const String& str_2);
bool operator!=(const String& str_1, const String& str_2);
String operator+(const char chr, const String &str_1);
String operator+(String str_1, const char chr);
String operator+(String str_1, const String &str_2);

String& String::operator+=(const String& str) {
  size_t prev_size_ = size_;
  size_ += str.size_;
  if (size_ >= capacity_) {
    changeCap();
  }
  std::copy(str.data_, str.data_ + str.size_, data_ + prev_size_);
  data_[size_] = '\0';
  return *this;
}

String& String::operator+=(const char& chr) {
  if (size_ + 1 >= capacity_) {
    changeCap();
  }
  data_[size_++] = chr;
  data_[size_] = '\0';
  return *this;
}

String& String::operator=(String str) {
  swap(str);
  return *this;
}

bool operator<(const String& str_1, const String& str_2) {
  if (str_1.length() < str_2.length()) {
    return true;
  } else if (str_1.length() > str_2.length()) {
    return false;
  } else {
    for (int i = 0; i < str_1.length(); ++ i) {
      if (str_1[i] < str_2[i]) {
        return true;
      }
      if (str_1[i] > str_2[i]) {
        return false;
      }
    }
  }
  return false;
}

bool operator>(const String& str_1, const String& str_2) {
  return str_2 < str_1;
}

bool operator<=(const String& str_1, const String& str_2) {
  return !(str_1 > str_2);
}
bool operator>=(const String& str_1, const String& str_2) {
  return !(str_1 < str_2);
}
bool operator==(const String& str_1, const String& str_2) {
  return !((str_1 < str_2) || (str_1 > str_2));
}

bool operator!=(const String& str_1, const String& str_2) {
  return !(str_1 == str_2);
}

size_t String::find(const String& substr_) const {
  int size_1 = substr_.size_;
  for (int i = 0; i + size_1 <= size_; ++i) {
    int flag = 1;
    for (int j = i; j < i + size_1; ++j) {
      if (substr_[j - i] != data_[j]) {
        flag = 0;
        break;
      }
    }
    if (flag == 1) {
      return i;
    }
  }
  return size_;
}
int String::rfind(const String& substr_) const {
  int size_1 = substr_.size_;
  for (int i = size_ - size_1; i >= 0; --i) {
    int flag = 1;
    for (int j = i; j < i + size_1; ++j) {
      if (substr_[j - i] != data_[j]) {
        flag = 0;
        break;
      }
    }
    if (flag == 1) {
      return i;
    }
  }
  return size_;
}

String operator+(String str_1, const String &str_2) {
  str_1 += str_2;
  return str_1;
}
String operator+(String str_1, const char chr) {
  const String str_2(1, chr);
  str_1 += str_2;
  return str_1;
}

String operator+(const char chr, const String &str_1) {
  return String(1, chr) + str_1;
}

String String::substr(int index, int count_index) const {
  String newstr(count_index, '\0');
  std::copy(data_ + index, data_ + index + count_index, newstr.data_);
  return newstr;
}

void String::shrink_to_fit() {
  capacity_ = size_;
  char *new_data_ = new char[capacity_ + 1];
  std::copy(data_, data_ + size_, new_data_);
  std::swap(data_, new_data_);
  delete[] new_data_;
  data_[size_] = '\0';
}

std::ostream &operator<<(std::ostream &stream, const String &str) {
  for (int i = 0; i < str.length(); ++i) {
    stream << str[i];
  }
  return stream;
}

std::istream &operator>>(std::istream &in, String &str) {
  str.clear();
  char chr;
  while (in.get(chr) && !isspace(chr) && chr != '\0') {
    str.push_back(chr);
  }
  return in;
}

