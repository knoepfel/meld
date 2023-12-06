#ifndef meld_core_glue_hpp
#define meld_core_glue_hpp

#include "meld/concurrency.hpp"
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
         std::vector<std::string>& errors,
         configuration const* config = nullptr) :
      graph_{g}, nodes_{nodes}, bound_obj_{std::move(bound_obj)}, errors_{errors}, config_{config}
    {
    }

    auto with(std::string name, auto f, concurrency c = concurrency::serial)
    {
      return bound_function{config_, std::move(name), bound_obj_, f, c, graph_, nodes_, errors_};
    }

    auto with(auto f, concurrency c = concurrency::serial) { return with(function_name(f), f, c); }

    auto output_with(std::string name, is_output_like auto f, concurrency c = concurrency::serial)
    {
      return output_creator{nodes_.register_output(errors_),
                            config_,
                            std::move(name),
                            graph_,
                            delegate(bound_obj_, f),
                            c};
    }
    auto output_with(is_output_like auto f, concurrency c = concurrency::serial)
    {
      return output_with(function_name(f), f, c);
    }

  private:
    tbb::flow::graph& graph_;
    node_catalog& nodes_;
    std::shared_ptr<T> bound_obj_;
    std::vector<std::string>& errors_;
    configuration const* config_;
  };

  template <typename T>
  class splitter_glue {
  public:
    splitter_glue(tbb::flow::graph& g,
                  node_catalog& nodes,
                  std::vector<std::string>& errors,
                  configuration const* config = nullptr) :
      graph_{g}, nodes_{nodes}, errors_{errors}, config_{config}
    {
    }

    auto declare_unfold(auto predicate, auto unfold, concurrency c)
    {
      return double_bound_function<T, decltype(predicate), decltype(unfold)>{
        config_,
        detail::stripped_name(boost::core::demangle(typeid(T).name())),
        predicate,
        unfold,
        c,
        graph_,
        nodes_,
        errors_};
    }

  private:
    tbb::flow::graph& graph_;
    node_catalog& nodes_;
    std::vector<std::string>& errors_;
    configuration const* config_;
  };
}

#endif /* meld_core_glue_hpp */
