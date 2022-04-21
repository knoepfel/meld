#ifndef meld_core_module_manager_hpp
#define meld_core_module_manager_hpp

#include "meld/core/module_owner.hpp"
#include "meld/core/module_worker.hpp"
#include "meld/core/source_owner.hpp"
#include "meld/core/source_worker.hpp"

#include "boost/json.hpp"

#include <string>

namespace meld {
  class module_manager {
  public:
    explicit module_manager(std::unique_ptr<source_worker> source,
                            std::map<std::string, module_worker_ptr> modules);

    source_worker& source();
    std::map<std::string, module_worker_ptr>& modules();

    // Testing interface
    template <typename Source>
    static module_manager make_with_source(boost::json::object const& config = {});

    template <typename T, typename... Ds>
    void add_module(std::string const& module_name, boost::json::object const& config = {});

    template <typename T>
    T const& get_source();

    template <typename T, typename... Ds>
    T const& get_module(std::string const& name);

  private:
    std::unique_ptr<source_worker> source_;
    std::map<std::string, module_worker_ptr> modules_;
  };

  template <typename Source>
  module_manager
  module_manager::make_with_source(boost::json::object const& config)
  {
    return module_manager{source_owner<Source>::create(config), {}};
  }

  template <typename T, typename... Ds>
  void
  module_manager::add_module(std::string const& module_name, boost::json::object const& config)
  {
    modules_.try_emplace(module_name, module_owner<T, Ds...>::create(config));
  }

  template <typename T>
  T const&
  module_manager::get_source()
  {
    auto user_source = dynamic_cast<source_owner<T> const*>(source_.get());
    if (!user_source) {
      throw std::runtime_error("The wrong type was specified for the source.");
    }
    return user_source->source();
  }

  template <typename T, typename... Ds>
  T const&
  module_manager::get_module(std::string const& name)
  {
    auto it = modules_.find(name);
    if (it == cend(modules_)) {
      throw std::runtime_error("A module with the name '" + name + "' has not been configured.");
    }

    auto user_module = dynamic_cast<module_owner<T, Ds...> const*>(it->second.get());
    if (!user_module) {
      throw std::runtime_error("The wrong type was specified for module '" + name + '.');
    }
    return user_module->module();
  }

}

#endif /* meld_core_module_manager_hpp */
