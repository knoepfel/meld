#ifndef sand_core_module_owner_hpp
#define sand_core_module_owner_hpp

#include "sand/core/module_worker.hpp"
#include "sand/core/node.hpp"

#include <typeinfo>

namespace sand {
  template <typename T, typename... Ds>
  class module_owner : public module_worker {
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
    do_process(node& data)
    {
      (process_<Ds>(data), ...);
    }

  private:
    T user_module;
  };
}

#endif
