#ifndef meld_core_module_owner_hpp
#define meld_core_module_owner_hpp

#include "meld/core/concurrency_tags.hpp"
#include "meld/core/uses_config.hpp"
#include "meld/graph/module_worker.hpp"
#include "meld/graph/node.hpp"
#include "meld/utilities/debug.hpp"

#include <memory>
#include <type_traits>

namespace meld {
  template <typename D, typename... Ds, typename Parent = typename D::parent_type>
  consteval bool
  supports_parent()
  {
    if (std::is_same<Parent, root_node_t>()) {
      return false;
    }
    return (std::is_same<Parent, Ds>() || ...);
  }

  template <typename T, typename... Ds>
  class module_owner : public module_worker {
  public:
    template <with_config U = T>
    explicit module_owner(boost::json::object const& config) : user_module{config}
    {
      if (auto deps = config.if_contains("dependencies")) {
        dependencies = value_to<std::vector<std::string>>(*deps);
      }
    }

    template <without_config U = T>
    explicit module_owner(boost::json::object const& config) : user_module{}
    {
      if (auto deps = config.if_contains("dependencies")) {
        dependencies = value_to<std::vector<std::string>>(*deps);
      }
    }

    static std::unique_ptr<module_worker>
    create(boost::json::object const& config)
    {
      return std::make_unique<module_owner<T, Ds...>>(config);
    }

    // For testing
    T const&
    module() const noexcept
    {
      return user_module;
    }

  private:
    std::size_t
    do_concurrency(transition_type const& tt) const final
    {
      auto const& [transition_name, stage] = tt;
      assert(stage != stage::flush);
      if (stage == stage::setup) {
        return setup_concurrencies.get(transition_name).value();
      }
      return process_concurrencies.get(transition_name).value();
    }

    std::vector<std::string>
    required_dependencies() const final
    {
      return dependencies;
    }

    std::vector<transition_type>
    supported_setup_transitions() const final
    {
      std::vector<transition_type> result;
      for (std::size_t i = 0; i < sizeof...(Ds); ++i) {
        if (auto const [name, has_setup] = setup_concurrencies.get(i); has_setup) {
          result.emplace_back(name, stage::setup);
        }
      }
      return result;
    }

    std::vector<transition_type>
    supported_process_transitions() const final
    {
      std::vector<transition_type> result;
      for (std::size_t i = 0; i < sizeof...(Ds); ++i) {
        if (auto const [name, has_process] = process_concurrencies.get(i); has_process) {
          result.emplace_back(name, stage::process);
        }
      }
      return result;
    }

    void
    do_process(stage const st, node& data) final
    {
      switch (st) {
      case stage::setup:
        (setup<Ds>(data) || ...);
        return;
      case stage::process:
        (process<Ds>(data) || ...);
        return;
      case stage::flush:
        return;
      }
    }

    template <typename D>
    void
    setup_data(D const& d)
    {
      constexpr auto tag = concurrency_tag_for_setup<T, D>();
      if constexpr (tag.is_specified) {
        user_module.setup(d, tag);
      }
      else {
        user_module.setup(d);
      }
    }

    template <typename D>
    void
    process_data(D const& d)
    {
      constexpr auto tag = concurrency_tag_for_process<T, D>();
      if constexpr (tag.is_specified) {
        user_module.process(d, tag);
      }
      else {
        user_module.process(d);
      }
    }

    // template <typename D>
    // bool
    // must_process(D*) const
    // {
    //   return false;
    // }

    template <typename D>
    bool
    setup(node& data)
    {
      if constexpr (!setup_concurrencies.template supports_level<D>()) {
        return true;
      }
      else {
        if (auto d = dynamic_cast<D*>(&data)) {
          setup_data(*d);
          return true;
        }
        return false;
      }
    }

    template <typename D>
    bool
    process(node& data)
    {
      if constexpr (!process_concurrencies.template supports_level<D>()) {
        return true;
      }
      else {
        if (auto d = dynamic_cast<D*>(&data)) {
          process_data(*d);
          return true;
        }
        return false;
      }
    }

    T user_module;
    std::vector<std::string> dependencies;
    static constexpr concurrencies<stage::setup, T, Ds...> setup_concurrencies;
    static constexpr concurrencies<stage::process, T, Ds...> process_concurrencies;
  };
}

#endif /* meld_core_module_owner_hpp */
