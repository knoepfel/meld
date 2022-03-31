#ifndef sand_core_module_hh
#define sand_core_module_hh

#include "sand/core/node.hpp"

#include <typeinfo>

namespace sand {
  template <typename T, typename... Ds>
  class module {
    template <typename D>
    void
    process_(node& data)
    {
      if (typeid(data) != typeid(D)) {
        return;
      }
      user_module.process(static_cast<D&>(data));
    }

  public:
    void
    process(node& data)
    {
      (process_<Ds>(data), ...);
    }

  private:
    T user_module;
  };
}

#endif
