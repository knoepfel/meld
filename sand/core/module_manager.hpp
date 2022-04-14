#ifndef sand_core_module_manager_hpp
#define sand_core_module_manager_hpp

#include "sand/core/module_owner.hpp"
#include "sand/core/module_worker.hpp"
#include "sand/core/source_worker.hpp"

#include <string>

namespace sand {
  class module_manager {
  public:
    explicit module_manager(std::unique_ptr<source_worker> source,
                            std::map<std::string, module_worker_ptr> modules);
    source_worker& source();
    std::map<std::string, module_worker_ptr>& modules();

    template <typename T, typename... Ds>
    T const&
    get_module(std::string const& name)
    {
      auto it = modules_.find(name);
      if (it == cend(modules_)) {
        throw std::runtime_error("A module with the name '" + name + "' has not been configured.");
      }

      auto user_module = dynamic_cast<module_owner<T, Ds...> const*>(it->second);
      if (!user_module) {
        throw std::runtime_error("The wrong type was specified for module '" + name + '.');
      }
      return *user_module;
    }

  private:
    std::unique_ptr<source_worker> source_;
    std::map<std::string, module_worker_ptr> modules_;
  };
}

#endif /* sand_core_module_manager_hpp */
