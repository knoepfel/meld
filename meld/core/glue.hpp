#ifndef meld_core_glue_hpp
#define meld_core_glue_hpp

#include "meld/core/bound_function.hpp"
#include "meld/core/concepts.hpp"
#include "meld/core/double_bound_function.hpp"
#include "meld/core/node_catalog.hpp"
#include "meld/core/registrar.hpp"
#include "meld/metaprogramming/delegate.hpp"
#include "meld/metaprogramming/function_name.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <memory>
#include <string>
#include <tuple>
#include <utility>

namespace meld {

  // ==============================================================================
  // Registering user functions

  template <typename T>
  class glue {
  public:
    glue(tbb::flow::graph& g,
         node_catalog& nodes,
         std::shared_ptr<T> bound_obj,
         std::vector<std::string>& errors) :
      graph_{g}, nodes_{nodes}, bound_obj_{std::move(bound_obj)}, errors_{errors}
    {
    }

    auto with(std::string name, auto f)
    {
      return bound_function{nullptr, std::move(name), bound_obj_, f, graph_, nodes_, errors_};
    }

    auto with(auto f) { return with(function_name(f), f); }

    auto declare_output(std::string name, is_output_like auto f)
    {
      return output_creator{
        nodes_.register_output(errors_), nullptr, std::move(name), graph_, delegate(bound_obj_, f)};
    }
    auto declare_output(auto f) { return declare_output(function_name(f), f); }

    auto declare_reduction(std::string name, auto f, auto&&... init_args)
    {
      return incomplete_reduction{nodes_.register_reduction(errors_),
                                  nullptr,
                                  std::move(name),
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
    tbb::flow::graph& graph_;
    node_catalog& nodes_;
    std::shared_ptr<T> bound_obj_;
    std::vector<std::string>& errors_;
  };

  template <typename T>
  class splitter_glue {
  public:
    splitter_glue(tbb::flow::graph& g, node_catalog& nodes, std::vector<std::string>& errors) :
      graph_{g}, nodes_{nodes}, errors_{errors}
    {
    }

    auto declare_unfold(auto predicate, auto unfold)
    {
      return double_bound_function<T, decltype(predicate), decltype(unfold)>{
        nullptr,
        detail::stripped_name(boost::core::demangle(typeid(T).name())),
        predicate,
        unfold,
        graph_,
        nodes_,
        errors_};
    }

  private:
    tbb::flow::graph& graph_;
    node_catalog& nodes_;
    std::vector<std::string>& errors_;
  };
}

#endif /* meld_core_glue_hpp */
