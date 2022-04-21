#ifndef test_debug_hpp
#define test_debug_hpp

#include <iostream>
#include <sstream>

namespace meld::test {
  template <typename... Args>
  void
  debug(Args&&... args)
  {
    std::ostringstream oss;
    ((oss << std::forward<Args>(args)), ...) << '\n';
    std::cout << oss.str() << std::flush;
  }
}

#endif /* test_debug_hpp */
