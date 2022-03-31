#ifndef sand_core_module_owner_hpp
#define sand_core_module_owner_hpp

#include "sand/core/module_worker.hpp"
#include "sand/core/node.hpp"

#include <memory>
#include <typeinfo>

namespace sand {
  template <typename T, typename... Ds>
  class module_owner : public module_worker {
  public:
    static std::unique_ptr<module_worker>
    create()
    {
      return std::make_unique<module_owner<T, Ds...>>();
    }

  private:
    template <typename D>
    void
    process_(node& data)
    {
      if (typeid(data) != typeid(D)) {
        return;
      }
      user_module.process(static_cast<D&>(data));
    }

    void
    do_process(node& data) final
    {
      (process_<Ds>(data), ...);
    }

    T user_module;
  };
}

#endif
