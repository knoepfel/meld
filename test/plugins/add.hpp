#ifndef test_plugins_add_hpp
#define test_plugins_add_hpp

#include <cassert>

namespace test {
  constexpr int add(int i, int j) { return i + j; }
  void verify_zero(int sum) noexcept { assert(sum == 0); }
}

#endif /* test_plugins_add_hpp */
