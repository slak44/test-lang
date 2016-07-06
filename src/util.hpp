#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>
#include <typeinfo>
#include <functional>
#include <cxxabi.h>

#define abs(x) (x < 0 ? -x : x)
#define UNUSED(expr) (void)(expr)
#define ALL(container) std::begin(container), std::end(container)

typedef long long int64;
typedef unsigned long long uint64;
typedef unsigned int uint;

template<typename T>
void println(T thing) {
  std::cout << thing << std::endl;
}

template<typename T>
void print(T thing) {
  std::cout << thing;
}

template<typename T, typename... Args>
void println(T thing, Args... args) {
  if (sizeof...(args) == 1) {
    print(thing);
    print(" ");
    print(args...);
    std::cout << std::endl;
    return;
  }
  print(thing);
  print(" ");
  print(args...);
}

template<typename T, typename... Args>
void print(T thing, Args... args) {
  print(thing);
  print(" ");
  print(args...);
}

template<typename T>
bool contains(std::vector<T> vec, T item) {
  return std::find(vec.begin(), vec.end(), item) != vec.end();
}

std::vector<std::string> split(const std::string& str, char delim) {
  std::vector<std::string> vec {};
  std::stringstream ss(str);
  std::string item;
  while (getline(ss, item, delim)) vec.push_back(item);
  return vec;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
  for (auto obj : vec) {
    os << obj << " ";
  }
  return os;
}

template<typename T>
struct PtrUtil {
  typedef std::unique_ptr<T> U;
  typedef std::shared_ptr<T> Link;
  typedef std::weak_ptr<T> WeakLink;
  
  template<typename... Args>
  static U unique(Args... args) {
    return std::make_unique<T>(args...);
  }
  
  template<typename... Args>
  static Link make(Args... args) {
    return std::make_shared<T>(args...);
  }
  
  template<typename U>
  static inline bool isSameType(std::shared_ptr<U> link) {
    return typeid(T) == typeid(*link);
  }
  
  template<typename U>
  static inline Link dynPtrCast(std::shared_ptr<U> link) {
    return std::dynamic_pointer_cast<T>(link);
  }
};

template<typename T>
struct VectorHash {
  std::size_t operator()(const std::vector<T>& vec) const {
    std::size_t seed = vec.size();
    std::hash<T> hash;
    for (const T& i : vec) {
      seed ^= hash(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

std::string getAddressStringFrom(const void* ptr) {
  std::stringstream ss;
  ss << "0x" << std::hex << reinterpret_cast<std::uintptr_t>(ptr);
  return ss.str();
}

std::string demangle(std::string name) {
  int status;
  return abi::__cxa_demangle(name.c_str(), 0, 0, &status);
}

#endif
