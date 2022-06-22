#ifndef meld_utilities_demangle_symbol_hpp
#define meld_utilities_demangle_symbol_hpp

#include <string>
#include <typeinfo>

namespace meld {
  std::string demangle_symbol(char const* p);
  std::string demangle_symbol(std::type_info const& type);
}

#endif /* meld_utilities_demangle_symbol_hpp */
