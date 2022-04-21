#ifndef meld_core_uses_config_hpp
#define meld_core_uses_config_hpp

#include "boost/json.hpp"

#include <type_traits>

namespace meld {
  template <typename Module>
  concept with_config = std::is_constructible<Module, boost::json::object const&>();

  template <typename Module>
  concept without_config = !with_config<Module>;
}

#endif /* meld_core_uses_config_hpp */
