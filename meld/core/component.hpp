#ifndef meld_core_component_hpp
#define meld_core_component_hpp

#include "meld/core/concepts.hpp"
#include "meld/core/declared_filter.hpp"
#include "meld/core/declared_monitor.hpp"
#include "meld/core/declared_output.hpp"
#include "meld/core/declared_reduction.hpp"
#include "meld/core/declared_splitter.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/core/registrar.hpp"
#include "meld/metaprogramming/delegate.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <concepts>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

namespace meld {

  // ==============================================================================
  // Registering user functions

  template <typename T>
  class component {
  public:
    template <typename>
    friend class component;

    component(tbb::flow::graph& g,
              declared_filters& filters,
              declared_monitors& monitors,
              declared_outputs& outputs,
              declared_reductions& reductions,
              declared_splitters& splitters,
              declared_transforms& transforms)
      requires(std::same_as<T, void_tag>)
      :
      graph_{g},
      filters_{filters},
      monitors_{monitors},
      outputs_{outputs},
      reductions_{reductions},
      splitters_{splitters},
      transforms_{transforms}
    {
    }

    template <typename U, typename... Args>
    component<U> bind_to(Args&&... args)
    {
      return component<U>{graph_,
                          filters_,
                          monitors_,
                          outputs_,
                          reductions_,
                          splitters_,
                          transforms_,
                          std::make_shared<U>(std::forward<Args>(args)...)};
    }

    auto declare_filter(std::string name, auto f)
    {
      return incomplete_filter{registrar{filters_}, move(name), graph_, delegate(bound_obj_, f)};
    }

    auto declare_monitor(std::string name, auto f)
    {
      return incomplete_monitor{registrar{monitors_}, move(name), graph_, delegate(bound_obj_, f)};
    }

    auto declare_output(std::string name, is_output_like auto f)
    {
      return output_creator{registrar{outputs_}, move(name), graph_, delegate(bound_obj_, f)};
    }

    auto declare_reduction(std::string name, auto f, auto&&... init_args)
    {
      return incomplete_reduction{registrar{reductions_},
                                  move(name),
                                  graph_,
                                  delegate(bound_obj_, f),
                                  std::make_tuple(std::forward<decltype(init_args)>(init_args)...)};
    }

    auto declare_splitter(std::string name, auto f)
    {
      return incomplete_splitter{
        registrar{splitters_}, move(name), graph_, delegate(bound_obj_, f)};
    }

    auto declare_transform(std::string name, auto f)
    {
      return incomplete_transform{
        registrar{transforms_}, move(name), graph_, delegate(bound_obj_, f)};
    }

  private:
    component(tbb::flow::graph& g,
              declared_filters& filters,
              declared_monitors& monitors,
              declared_outputs& outputs,
              declared_reductions& reductions,
              declared_splitters& splitters,
              declared_transforms& transforms,
              std::shared_ptr<T> bound_obj)
      requires(not std::same_as<T, void_tag>)
      :
      graph_{g},
      filters_{filters},
      monitors_{monitors},
      outputs_{outputs},
      reductions_{reductions},
      splitters_{splitters},
      transforms_{transforms},
      bound_obj_{bound_obj}
    {
    }

    tbb::flow::graph& graph_;
    declared_filters& filters_;
    declared_monitors& monitors_;
    declared_outputs& outputs_;
    declared_reductions& reductions_;
    declared_splitters& splitters_;
    declared_transforms& transforms_;
    std::shared_ptr<T> bound_obj_;
  };
}

#endif /* meld_core_component_hpp */
