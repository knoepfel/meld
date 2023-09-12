#ifndef meld_core_double_bound_function_hpp
#define meld_core_double_bound_function_hpp

#include "meld/concurrency.hpp"
#include "meld/configuration.hpp"
#include "meld/core/concepts.hpp"
#include "meld/core/declared_filter.hpp"
#include "meld/core/declared_monitor.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/core/input_arguments.hpp"
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
    using input_parameter_types = constructor_parameter_types<Object>;
    static_assert(
      std::same_as<function_parameter_types<Predicate>, function_parameter_types<Unfold>>);

  public:
    static constexpr auto N = number_parameters<Predicate>;

    double_bound_function(configuration const* config,
                          std::string name,
                          Predicate predicate,
                          Unfold unfold,
                          concurrency c,
                          tbb::flow::graph& g,
                          node_catalog& nodes,
                          std::vector<std::string>& errors) :
      node_options_t{config},
      name_{std::move(name)},
      predicate_{std::move(predicate)},
      unfold_{std::move(unfold)},
      concurrency_{c.value},
      graph_{g},
      nodes_{nodes},
      errors_{errors}
    {
    }

    auto split(std::array<specified_label, N> input_args)
    {
      auto processed_input_args =
        form_input_arguments<input_parameter_types>(std::move(input_args));

      return partial_splitter<Object, Predicate, Unfold, decltype(processed_input_args)>{
        nodes_.register_splitter(errors_),
        std::move(name_),
        concurrency_,
        node_options_t::release_preceding_filters(),
        graph_,
        std::move(predicate_),
        std::move(unfold_),
        std::move(processed_input_args)};
    }

    auto split(label_compatible auto... input_args)
    {
      static_assert(N == sizeof...(input_args),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      return split({specified_label{std::forward<decltype(input_args)>(input_args)}...});
    }

  private:
    std::string name_;
    Predicate predicate_;
    Unfold unfold_;
    std::size_t concurrency_;
    tbb::flow::graph& graph_;
    node_catalog& nodes_;
    std::vector<std::string>& errors_;
  };
}

#endif /* meld_core_double_bound_function_hpp */
