#ifndef test_plugins_add_hpp
#define test_plugins_add_hpp

#include <cassert>

namespace test {
  constexpr int add(int i, int j) { return i + j; }
  bool verify_zero(int sum)
  {
    assert(sum == 0);
    return true;
  }
}

#endif /* test_plugins_add_hpp */
