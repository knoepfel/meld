#ifndef sand_core_uses_config_hpp
#define sand_core_uses_config_hpp

#include "boost/json.hpp"

#include <type_traits>

namespace sand {
  template <typename Module>
  concept with_config = std::is_constructible<Module, boost::json::object const&>();

  template <typename Module>
  concept without_config = !with_config<Module>;
}

#endif /* sand_core_uses_config_hpp */
