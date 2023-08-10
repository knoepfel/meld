#ifndef meld_core_double_bound_function_hpp
#define meld_core_double_bound_function_hpp

#include "meld/configuration.hpp"
#include "meld/core/concepts.hpp"
#include "meld/core/declared_filter.hpp"
#include "meld/core/declared_monitor.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/core/node_catalog.hpp"
#include "meld/core/node_options.hpp"
#include "meld/metaprogramming/delegate.hpp"
#include "meld/metaprogramming/type_deduction.hpp"

#include <concepts>
#include <functional>
#include <memory>

namespace meld {

  template <typename Object, typename Predicate, typename Unfold>
  class double_bound_function :
    public node_options<double_bound_function<Object, Predicate, Unfold>> {
    using node_options_t = node_options<double_bound_function<Object, Predicate, Unfold>>;
    using input_parameter_types = parameter_types<Predicate>;

  public:
    static constexpr auto N = number_parameters<Predicate>;

    double_bound_function(configuration const* config,
                          std::string name,
                          Predicate predicate,
                          Unfold unfold,
                          tbb::flow::graph& g,
                          node_catalog& nodes,
                          std::vector<std::string>& errors) :
      node_options_t{config},
      name_{std::move(name)},
      predicate_{std::move(predicate)},
      unfold_{std::move(unfold)},
      graph_{g},
      nodes_{nodes},
      errors_{errors}
    {
    }

    template <typename... InputArgs>
    auto split(std::tuple<InputArgs...> input_args)
    {
      static_assert(N == sizeof...(InputArgs),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      auto processed_input_args =
        detail::form_input_arguments<input_parameter_types>(std::move(input_args));
      auto product_names = detail::port_names(processed_input_args);

      return partial_splitter<Object, Predicate, Unfold, decltype(processed_input_args)>{
        nodes_.register_splitter(errors_),
        std::move(name_),
        node_options_t::concurrency(),
        node_options_t::release_preceding_filters(),
        std::vector<std::string>{},
        graph_,
        std::move(predicate_),
        std::move(unfold_),
        std::move(processed_input_args),
        std::move(product_names)};
    }

    auto split(std::convertible_to<std::string> auto... input_args)
    {
      return split(std::make_tuple(react_to(std::forward<decltype(input_args)>(input_args))...));
    }

  private:
    std::string name_;
    Predicate predicate_;
    Unfold unfold_;
    tbb::flow::graph& graph_;
    node_catalog& nodes_;
    std::vector<std::string>& errors_;
  };
}

#endif /* meld_core_double_bound_function_hpp */
