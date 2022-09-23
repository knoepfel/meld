#ifndef meld_utilities_debug_hpp
#define meld_utilities_debug_hpp

#include <iostream>
#include <sstream>

namespace meld {
  template <typename... Args>
  void debug(Args&&... args)
  {
    std::ostringstream oss;
    ((oss << std::forward<Args>(args)), ...) << '\n';
    std::cout << oss.str() << std::flush;
  }
}

#endif /* meld_utilities_debug_hpp */
