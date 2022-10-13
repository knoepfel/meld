#ifndef meld_utilities_debug_hpp
#define meld_utilities_debug_hpp

#include "spdlog/spdlog.h"

namespace meld {
  template <typename... Args>
  void debug(Args&&... args)
  {
    auto braces_for = [](auto&) -> std::string { return "{}"; };
    std::string format_string;
    (format_string.append(braces_for(args)), ...);
    spdlog::debug(format_string, args...);
  }
}

#endif /* meld_utilities_debug_hpp */
