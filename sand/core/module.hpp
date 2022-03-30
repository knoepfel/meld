#ifndef sand_core_module_hh
#define sand_core_module_hh

#include "sand/core/node.hpp"

#include <iostream>
#include <typeindex>
#include <typeinfo>
#include <vector>

namespace sand {
  template <typename T, typename D>
  class module {
  public:
    module() :supported_data_types{std::type_index(typeid(D))} {}

    void
    process(node& data)
    {
      auto it = std::find(
        cbegin(supported_data_types), cend(supported_data_types), std::type_index(typeid(data)));
      if (it == cend(supported_data_types)) {
        return;
      }
      user_module.process(static_cast<D&>(data));
    }

  private:
    T user_module;
    std::vector<std::type_index> supported_data_types;
  };
}

#endif
