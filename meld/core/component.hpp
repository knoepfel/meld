#ifndef meld_core_component_hpp
#define meld_core_component_hpp

#include "meld/core/concepts.hpp"
#include "meld/core/declared_filter.hpp"
#include "meld/core/declared_monitor.hpp"
#include "meld/core/declared_output.hpp"
#include "meld/core/declared_reduction.hpp"
#include "meld/core/declared_splitter.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/utilities/type_deduction.hpp"

#include "oneapi/tbb/flow_graph.h"
#include "oneapi/tbb/global_control.h"

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
              declared_transforms& transforms) requires(std::same_as<T, void_tag>) :
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

    auto declare_filter(std::string name, is_filter_like auto f)
    {
      return incomplete_filter{*this, name, graph_, delegate(bound_obj_, f)};
    }

    auto declare_monitor(std::string name, is_monitor_like auto f)
    {
      return incomplete_monitor{*this, name, graph_, delegate(bound_obj_, f)};
    }

    auto declare_output(std::string name, is_output_like auto f)
    {
      return incomplete_output{*this, name, graph_, delegate(bound_obj_, f)};
    }

    auto declare_reduction(std::string name,
                           is_reduction_like auto f,
                           auto&&... init_args) requires std::same_as<T, void_tag>
    {
      return incomplete_reduction{*this,
                                  name,
                                  graph_,
                                  delegate(bound_obj_, f),
                                  std::make_tuple(std::forward<decltype(init_args)>(init_args)...)};
    }

    auto declare_splitter(std::string name, is_splitter_like auto f)
    {
      return incomplete_splitter{*this, name, graph_, delegate(bound_obj_, f)};
    }

    auto declare_transform(std::string name, is_transform_like auto f)
    {
      return incomplete_transform{*this, name, graph_, delegate(bound_obj_, f)};
    }

    // Expert-use only
    void add_filter(std::string const& name, declared_filter_ptr ptr)
    {
      filters_.try_emplace(name, std::move(ptr));
    }

    void add_monitor(std::string const& name, declared_monitor_ptr ptr)
    {
      monitors_.try_emplace(name, std::move(ptr));
    }

    void add_output(std::string const& name, declared_output_ptr ptr)
    {
      outputs_.try_emplace(name, std::move(ptr));
    }

    void add_reduction(std::string const& name, declared_reduction_ptr ptr)
    {
      reductions_.try_emplace(name, std::move(ptr));
    }

    void add_splitter(std::string const& name, declared_splitter_ptr ptr)
    {
      splitters_.try_emplace(name, std::move(ptr));
    }

    void add_transform(std::string const& name, declared_transform_ptr ptr)
    {
      transforms_.try_emplace(name, std::move(ptr));
    }

  private:
    component(tbb::flow::graph& g,
              declared_filters& filters,
              declared_monitors& monitors,
              declared_outputs& outputs,
              declared_reductions& reductions,
              declared_splitters& splitters,
              declared_transforms& transforms,
              std::shared_ptr<T> bound_obj) requires(not std::same_as<T, void_tag>) :
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
