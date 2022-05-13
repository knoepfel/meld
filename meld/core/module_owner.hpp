#ifndef meld_core_module_owner_hpp
#define meld_core_module_owner_hpp

#include "meld/core/concurrency_tags.hpp"
#include "meld/core/uses_config.hpp"
#include "meld/graph/data_node.hpp"
#include "meld/graph/module_worker.hpp"
#include "meld/graph/serial_node.hpp"
#include "meld/utilities/debug.hpp"

#include <memory>
#include <type_traits>

namespace meld {

  template <std::size_t... Is>
  decltype(auto)
  to_tuple(serializers& serialized_resources,
           std::span<std::string_view const> names,
           std::index_sequence<Is...>)
  {
    return serialized_resources.get(names[Is]...);
  }

  template <std::size_t N, typename FT>
  std::shared_ptr<module_node>
  module_node_for(tbb::flow::graph& g,
                  std::size_t concurrency,
                  FT ft,
                  serializers& serialized_resources,
                  std::span<std::string_view const> resource_names)
  {
    if constexpr (N == 0) {
      return std::make_shared<serial_node<data_node_ptr, 0>>(g, concurrency, std::move(ft));
    }
    else {
      return std::make_shared<serial_node<data_node_ptr, N>>(
        g,
        to_tuple(serialized_resources, resource_names, std::make_index_sequence<N>{}),
        std::move(ft));
    }
  }

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
    std::shared_ptr<module_node>
    do_create_worker_node(tbb::flow::graph& g,
                          transition_type const& tt,
                          serializers& serialized_resources) override
    {
      auto const& [level_name, stage] = tt;
      if (stage == stage::setup) {
        return create_setup_node<Ds...>(g, level_name, serialized_resources);
      }
      return create_process_node<Ds...>(g, level_name, serialized_resources);
    }

    template <typename Head, typename... Tail>
    std::shared_ptr<module_node>
    create_setup_node(tbb::flow::graph& g,
                      std::string_view const level_name,
                      serializers& serialized_resources)
    {
      constexpr auto concurrency = setup_concurrencies.template get<Head>();
      constexpr auto num_resources = concurrency.resource_names.size();
      if constexpr (!concurrency.value) {
        return nullptr;
      }
      else {
        if (Head::name() == level_name) {
          auto const [name, value, resource_names] = concurrency;
          std::size_t concurrency_value = *value == 0 ? tbb::flow::unlimited : *value;
          return module_node_for<num_resources>(
            g,
            concurrency_value,
            [this](data_node_ptr data_ptr) mutable {
              auto d = static_pointer_cast<Head>(data_ptr);
              setup(*d);
              return data_ptr;
            },
            serialized_resources,
            resource_names);
        }
        if constexpr (sizeof...(Tail) > 0) {
          return create_setup_node<Tail...>(g, level_name, serialized_resources);
        }
        return nullptr;
      }
    }

    template <typename Head, typename... Tail>
    std::shared_ptr<module_node>
    create_process_node(tbb::flow::graph& g,
                        std::string_view const level_name,
                        serializers& serialized_resources)
    {
      constexpr auto concurrency = process_concurrencies.template get<Head>();
      constexpr auto num_resources = concurrency.resource_names.size();
      if constexpr (!concurrency.value) {
        return nullptr;
      }
      else {
        if (Head::name() == level_name) {
          auto const [name, value, resource_names] = concurrency;
          std::size_t concurrency_value = *value == 0 ? tbb::flow::unlimited : *value;
          return module_node_for<num_resources>(
            g,
            concurrency_value,
            [this](data_node_ptr data_ptr) mutable {
              auto d = static_pointer_cast<Head>(data_ptr);
              process(*d);
              return data_ptr;
            },
            serialized_resources,
            resource_names);
        }
        if constexpr (sizeof...(Tail) > 0) {
          return create_process_node<Tail...>(g, level_name, serialized_resources);
        }
        return nullptr;
      }
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
        if (auto const [name, has_setup, _] = setup_concurrencies.get(i); has_setup) {
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
        if (auto const [name, has_process, _] = process_concurrencies.get(i); has_process) {
          result.emplace_back(name, stage::process);
        }
      }
      return result;
    }

    template <typename D>
    void
    setup(D const& d)
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
    process(D const& d)
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

    T user_module;
    std::vector<std::string> dependencies;
    static constexpr concurrencies<stage::setup, T, Ds...> setup_concurrencies;
    static constexpr concurrencies<stage::process, T, Ds...> process_concurrencies;
  };
}

#endif /* meld_core_module_owner_hpp */
