#ifndef test_plugins_add_hpp
#define test_plugins_add_hpp

#include <cassert>

namespace test {
  constexpr int add(int i, int j) { return i + j; }
  void verify(int sum, int expected) noexcept { assert(sum == expected); }
}

#endif /* test_plugins_add_hpp */
