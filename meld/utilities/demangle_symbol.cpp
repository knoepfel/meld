#include "meld/utilities/demangle_symbol.hpp"

#include <cstdlib>
#include <cxxabi.h>

namespace meld {
  std::string demangle_symbol(char const* p)
  {
    std::string res;
    int status;
    char* demangled = abi::__cxa_demangle(p, nullptr, nullptr, &status);
    if (demangled) {
      res = demangled;
      std::free(demangled);
    }
    return res;
  }

  std::string demangle_symbol(std::type_info const& type) { return demangle_symbol(type.name()); }
}
