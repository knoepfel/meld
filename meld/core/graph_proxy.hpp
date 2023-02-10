#ifndef meld_core_graph_proxy_hpp
#define meld_core_graph_proxy_hpp

#include "meld/configuration.hpp"
#include "meld/core/bound_function.hpp"
#include "meld/core/concepts.hpp"
#include "meld/core/node_catalog.hpp"
#include "meld/core/registrar.hpp"
#include "meld/metaprogramming/delegate.hpp"
#include "meld/metaprogramming/function_name.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <concepts>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

  // ==============================================================================
  // Registering user functions

  template <typename T>
  class graph_proxy {
    auto qualified(std::string const& name)
    {
      return config_->get<std::string>("module_label") + ":" + name;
    }

  public:
    template <typename>
    friend class graph_proxy;

    graph_proxy(configuration const& config,
                tbb::flow::graph& g,
                node_catalog& nodes,
                std::vector<std::string>& errors)
      requires(std::same_as<T, void_tag>)
      : config_{&config}, graph_{g}, nodes_{nodes}, errors_{errors}
    {
    }

    template <typename U, typename... Args>
    graph_proxy<U> make(Args&&... args)
    {
      return graph_proxy<U>{
        config_, graph_, nodes_, std::make_shared<U>(std::forward<Args>(args)...), errors_};
    }

    auto with(std::string name, auto f)
    {
      return bound_function{config_, qualified(name), bound_obj_, f, graph_, nodes_, errors_};
    }

    auto with(auto f) { return with(function_name(f), f); }

    auto declare_output(std::string name, is_output_like auto f)
    {
      return output_creator{
        nodes_.register_output(errors_), config_, qualified(name), graph_, delegate(bound_obj_, f)};
    }
    auto declare_output(auto f) { return declare_output(function_name(f), f); }

    auto declare_reduction(std::string name, auto f, auto&&... init_args)
    {
      return incomplete_reduction{nodes_.register_reduction(errors_),
                                  config_,
                                  qualified(name),
                                  graph_,
                                  delegate(bound_obj_, f),
                                  std::make_tuple(std::forward<decltype(init_args)>(init_args)...)};
    }
    auto declare_reduction(auto f, auto&&... init_args)
    {
      return declare_reduction(
        function_name(f), f, std::forward<decltype(init_args)>(init_args)...);
    }

  private:
    graph_proxy(configuration const* config,
                tbb::flow::graph& g,
                node_catalog& nodes,
                std::shared_ptr<T> bound_obj,
                std::vector<std::string>& errors)
      requires(not std::same_as<T, void_tag>)
      : config_{config}, graph_{g}, nodes_{nodes}, bound_obj_{bound_obj}, errors_{errors}
    {
    }

    configuration const* config_;
    tbb::flow::graph& graph_;
    node_catalog& nodes_;
    std::shared_ptr<T> bound_obj_;
    std::vector<std::string>& errors_;
  };
}

#endif /* meld_core_graph_proxy_hpp */
