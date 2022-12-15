#ifndef meld_core_module_proxy_hpp
#define meld_core_module_proxy_hpp

#include "meld/core/concepts.hpp"
#include "meld/core/declared_filter.hpp"
#include "meld/core/declared_monitor.hpp"
#include "meld/core/declared_output.hpp"
#include "meld/core/declared_reduction.hpp"
#include "meld/core/declared_splitter.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/core/registrar.hpp"
#include "meld/metaprogramming/delegate.hpp"

#include "boost/json.hpp"
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
  class module_proxy {
    auto qualified(std::string const& name)
    {
      return value_to<std::string>(config_->at("module_label")) + ":" + name;
    }

  public:
    template <typename>
    friend class module_proxy;

    module_proxy(boost::json::object const& config,
                 tbb::flow::graph& g,
                 declared_filters& filters,
                 declared_monitors& monitors,
                 declared_outputs& outputs,
                 declared_reductions& reductions,
                 declared_splitters& splitters,
                 declared_transforms& transforms,
                 std::vector<std::string>& errors)
      requires(std::same_as<T, void_tag>)
      :
      config_{&config},
      graph_{g},
      filters_{filters},
      monitors_{monitors},
      outputs_{outputs},
      reductions_{reductions},
      splitters_{splitters},
      transforms_{transforms},
      errors_{errors}
    {
    }

    template <typename U, typename... Args>
    module_proxy<U> make(Args&&... args)
    {
      return module_proxy<U>{config_,
                             graph_,
                             filters_,
                             monitors_,
                             outputs_,
                             reductions_,
                             splitters_,
                             transforms_,
                             std::make_shared<U>(std::forward<Args>(args)...),
                             errors_};
    }

    auto declare_filter(std::string name, auto f)
    {
      return incomplete_filter{
        registrar{filters_, errors_}, config_, qualified(name), graph_, delegate(bound_obj_, f)};
    }

    auto declare_monitor(std::string name, auto f)
    {
      return incomplete_monitor{
        registrar{monitors_, errors_}, config_, qualified(name), graph_, delegate(bound_obj_, f)};
    }

    auto declare_output(std::string name, is_output_like auto f)
    {
      return output_creator{
        registrar{outputs_, errors_}, config_, qualified(name), graph_, delegate(bound_obj_, f)};
    }

    auto declare_reduction(std::string name, auto f, auto&&... init_args)
    {
      return incomplete_reduction{registrar{reductions_, errors_},
                                  config_,
                                  qualified(name),
                                  graph_,
                                  delegate(bound_obj_, f),
                                  std::make_tuple(std::forward<decltype(init_args)>(init_args)...)};
    }

    auto declare_splitter(std::string name, auto f)
    {
      return incomplete_splitter{
        registrar{splitters_, errors_}, config_, qualified(name), graph_, delegate(bound_obj_, f)};
    }

    auto declare_transform(std::string name, auto f)
    {
      return incomplete_transform{
        registrar{transforms_, errors_}, config_, qualified(name), graph_, delegate(bound_obj_, f)};
    }

  private:
    module_proxy(boost::json::object const* config,
                 tbb::flow::graph& g,
                 declared_filters& filters,
                 declared_monitors& monitors,
                 declared_outputs& outputs,
                 declared_reductions& reductions,
                 declared_splitters& splitters,
                 declared_transforms& transforms,
                 std::shared_ptr<T> bound_obj,
                 std::vector<std::string>& errors)
      requires(not std::same_as<T, void_tag>)
      :
      config_{config},
      graph_{g},
      filters_{filters},
      monitors_{monitors},
      outputs_{outputs},
      reductions_{reductions},
      splitters_{splitters},
      transforms_{transforms},
      bound_obj_{bound_obj},
      errors_{errors}
    {
    }

    boost::json::object const* config_;
    tbb::flow::graph& graph_;
    declared_filters& filters_;
    declared_monitors& monitors_;
    declared_outputs& outputs_;
    declared_reductions& reductions_;
    declared_splitters& splitters_;
    declared_transforms& transforms_;
    std::shared_ptr<T> bound_obj_;
    std::vector<std::string>& errors_;
  };
}

#endif /* meld_core_module_proxy_hpp */
