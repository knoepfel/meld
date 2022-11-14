#ifndef meld_utilities_debug_hpp
#define meld_utilities_debug_hpp

#include "spdlog/spdlog.h"

namespace meld {
  template <typename... Args>
  void debug(Args&&... args)
  {
    auto braces_for = [](auto&) -> std::string { return "{}"; };
    std::string const fmt_str{("" + ... + braces_for(args))};
    spdlog::debug(fmt::runtime(fmt_str), args...);
  }
}

#endif /* meld_utilities_debug_hpp */
