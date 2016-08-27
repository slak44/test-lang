#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <set>
#include <memory>
#include <functional>

#include "utils/suppressWarnings.hpp"

#define UNUSED(expr) (void)(expr)
#define ALL(container) std::begin(container), std::end(container)

using uint = unsigned int;
using int64 = long long;
using uint64 = unsigned long long;

/// Represents a list of types. Used in multiple places
using TypeList = std::set<std::string>;

/// Print something to stdout with a trailing newline
template<typename T>
void println(T thing) {
  std::cout << thing << std::endl;
}

/// Print something to stdout
template<typename T>
void print(T thing) {
  std::cout << thing;
}

/// Print multiple things (with spaces in-between) to stdout
template<typename T, typename... Args>
void print(T thing, Args... args) {
  print(thing);
  print(" ");
  print(args...);
}

/// Print multiple things (with spaces in-between) to stdout with a trailing newline
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

/// Checks if the vector has the item in it
template<typename T>
bool contains(std::vector<T> vec, T item) {
  return std::find(vec.begin(), vec.end(), item) != vec.end();
}

/**
  \brief Split a string into substrings
  \param delim where to break the string
*/
std::vector<std::string> split(const std::string& str, char delim);

/// Allows the use of vectors as map keys
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

/// Smart pointer utils
template<typename T>
struct PtrUtil {
  /// Shared pointer
  using Link = std::shared_ptr<T>;
  /// Weak pointer
  using WeakLink = std::weak_ptr<T>;
  
  /**
    \brief Use RTTI to determine if 2 pointers have the same type.
    \tparam U type stored in the shared ptr to compare to
    
    Compares this PtrUtil's template type with this function's template type.
  */
  template<typename U>
  static inline bool isSameType(const std::shared_ptr<U>& link) {
    return typeid(T) == typeid(*link);
  }
  
  /**
    \brief Wrapper around std::dynamic_pointer_cast.
    \tparam U stored in the shared ptr to be casted
  */
  template<typename U>
  static inline Link dynPtrCast(const std::shared_ptr<U>& link) {
    return std::dynamic_pointer_cast<T>(link);
  }
  
  /**
    \brief Wrapper around std::static_pointer_cast.
    \tparam U stored in the shared ptr to be casted
  */
  template<typename U>
  static inline Link staticPtrCast(const std::shared_ptr<U>& link) {
    return std::static_pointer_cast<T>(link);
  }
};

/// Get a pretty address string from a pointer
std::string getAddressStringFrom(const void* ptr);

/// Identity function. Does literally nothing.
struct Identity {
  template<typename T>
  constexpr auto operator()(T&& v) const noexcept -> decltype(std::forward<T>(v)) {
    return std::forward<T>(v);
  }
};

static const auto defaultCollateCombine = [](std::string prev, std::string current) {return prev + ", " + current;};
#define COLLATE_TYPE const typename Container::value_type&

/**
  \brief Collates a bunch of objects from Container into a string
  \param c the Container with objects
  \param objToString lambda to convert the object to a string (by default uses Identity)
  \param combine reduce-style lambda
*/
template<typename Container>
std::string collate(Container c,
  std::function<std::string(COLLATE_TYPE)> objToString = Identity(),
  std::function<std::string(std::string, std::string)> combine = defaultCollateCombine
  ) {
  if (c.size() == 0) return "";
  if (c.size() == 1) return objToString(*c.begin());
  if (c.size() >= 2) {
    std::string str = objToString(*c.begin());
    std::for_each(++c.begin(), c.end(), [&str, &objToString, &combine](COLLATE_TYPE object) {
      str = combine(str, objToString(object));
    });
    return str;
  }
  throw std::logic_error("Size of container must be a positive integer");
}

#undef COLLATE_TYPE

/**
  \brief Convenience function
  \see collate
*/
std::string collateTypeList(TypeList typeList);

#endif