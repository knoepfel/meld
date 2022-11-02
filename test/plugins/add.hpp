#ifndef test_plugins_add_hpp
#define test_plugins_add_hpp

#include <cassert>

namespace test {
  constexpr int add(int i, int j) { return i + j; }

  struct verify {
    int expected;
    void check(int actual) const noexcept { assert(actual == expected); }
  };

  void verify_zero(int sum) noexcept { verify{0}.check(sum); }

}

#endif /* test_plugins_add_hpp */
