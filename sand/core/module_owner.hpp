#ifndef sand_core_module_owner_hpp
#define sand_core_module_owner_hpp

#include "sand/core/module_worker.hpp"
#include "sand/core/node.hpp"

#include <memory>

namespace sand {
  template <typename T, typename... Ds>
  class module_owner : public module_worker {
  public:
    explicit module_owner(boost::json::object const& config) : user_module{config} {}

    static std::unique_ptr<module_worker>
    create(boost::json::object const& config)
    {
      return std::make_unique<module_owner<T, Ds...>>(config);
    }

  private:
    template <typename D>
    void
    process_(node& data)
    {
      if (auto d = dynamic_cast<D*>(&data)) {
        user_module.process(*d);
      }
    }

    void
    do_process(node& data) final
    {
      (process_<Ds>(data), ...);
    }

    T user_module;
  };
}

#endif /* sand_core_module_owner_hpp */
