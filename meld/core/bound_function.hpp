#ifndef meld_core_bound_function_hpp
#define meld_core_bound_function_hpp

#include "meld/concurrency.hpp"
#include "meld/configuration.hpp"
#include "meld/core/concepts.hpp"
#include "meld/core/declared_filter.hpp"
#include "meld/core/declared_monitor.hpp"
#include "meld/core/declared_reduction.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/core/node_catalog.hpp"
#include "meld/core/node_options.hpp"
#include "meld/metaprogramming/delegate.hpp"
#include "meld/metaprogramming/type_deduction.hpp"

#include <concepts>
#include <functional>
#include <memory>

namespace meld {

  template <typename T, typename FT>
  class bound_function : public node_options<bound_function<T, FT>> {
    using node_options_t = node_options<bound_function<T, FT>>;
    using input_parameter_types = function_parameter_types<FT>;

  public:
    static constexpr auto N = number_parameters<FT>;

    bound_function(configuration const* config,
                   std::string name,
                   std::shared_ptr<T> obj,
                   FT f,
                   concurrency c,
                   tbb::flow::graph& g,
                   node_catalog& nodes,
                   std::vector<std::string>& errors) :
      node_options_t{config},
      name_{std::move(name)},
      obj_{obj},
      ft_{std::move(f)},
      concurrency_{c},
      graph_{g},
      nodes_{nodes},
      errors_{errors}
    {
    }

    auto& filter(std::array<specified_label, N> input_args)
      requires is_filter_like<FT>
    {
      auto processed_input_args =
        form_input_arguments<input_parameter_types>(std::move(input_args));
      nodes_.register_filter(errors_).set([this, inputs = std::move(processed_input_args)] {
        auto product_names = detail::port_names(inputs);
        auto function_closure = delegate(obj_, ft_);

        return std::make_unique<
          complete_filter<decltype(function_closure), decltype(processed_input_args)>>(
          std::move(name_),
          concurrency_.value,
          node_options_t::release_preceding_filters(),
          std::vector<std::string>{},
          graph_,
          std::move(function_closure),
          std::move(inputs),
          std::move(product_names));
      });
      return *this;
    }

    auto& monitor(std::array<specified_label, N> input_args)
      requires is_monitor_like<FT>
    {
      auto processed_input_args =
        form_input_arguments<input_parameter_types>(std::move(input_args));
      nodes_.register_monitor(errors_).set([this, inputs = std::move(processed_input_args)] {
        auto product_names = detail::port_names(inputs);
        auto function_closure = delegate(obj_, ft_);

        return std::make_unique<
          complete_monitor<decltype(function_closure), decltype(processed_input_args)>>(
          std::move(name_),
          concurrency_.value,
          node_options_t::release_preceding_filters(),
          std::vector<std::string>{},
          graph_,
          std::move(function_closure),
          std::move(inputs),
          std::move(product_names));
      });
      return *this;
    }

    auto transform(std::array<specified_label, N> input_args)
      requires is_transform_like<FT>
    {
      auto processed_input_args =
        form_input_arguments<input_parameter_types>(std::move(input_args));
      auto product_names = detail::port_names(processed_input_args);
      auto function_closure = delegate(obj_, ft_);

      return pre_transform<decltype(function_closure), decltype(processed_input_args)>{
        nodes_.register_transform(errors_),
        std::move(name_),
        concurrency_.value,
        node_options_t::release_preceding_filters(),
        graph_,
        std::move(function_closure),
        std::move(processed_input_args),
        std::move(product_names)};
    }

    auto reduce(std::array<specified_label, N - 1> input_args)
      requires is_reduction_like<FT>
    {
      using all_but_first = skip_first_type<input_parameter_types>;
      auto processed_input_args = form_input_arguments<all_but_first>(std::move(input_args));
      auto product_names = detail::port_names(processed_input_args);
      auto function_closure = delegate(obj_, ft_);
      return pre_reduction<decltype(function_closure), decltype(processed_input_args)>{
        nodes_.register_reduction(errors_),
        std::move(name_),
        concurrency_.value,
        node_options_t::release_preceding_filters(),
        node_options_t::release_store_names(),
        graph_,
        std::move(function_closure),
        std::move(processed_input_args),
        std::move(product_names)};
    }

    auto filter(std::convertible_to<std::string> auto... input_args)
    {
      static_assert(N == sizeof...(input_args),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      return filter({specified_label{std::forward<decltype(input_args)>(input_args)}...});
    }

    auto monitor(std::convertible_to<std::string> auto... input_args)
    {
      static_assert(N == sizeof...(input_args),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      return monitor({specified_label{std::forward<decltype(input_args)>(input_args)}...});
    }

    auto transform(std::convertible_to<std::string> auto... input_args)
    {
      static_assert(N == sizeof...(input_args),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      return transform({specified_label{std::forward<decltype(input_args)>(input_args)}...});
    }

    auto reduce(std::convertible_to<std::string> auto... input_args)
    {
      static_assert(N - 1 == sizeof...(input_args),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      return reduce({specified_label{std::forward<decltype(input_args)>(input_args)}...});
    }

  private:
    std::string name_;
    std::shared_ptr<T> obj_;
    FT ft_;
    concurrency concurrency_;
    tbb::flow::graph& graph_;
    node_catalog& nodes_;
    std::vector<std::string>& errors_;
  };

  template <typename T, typename FT>
  bound_function(configuration const*, std::string, std::shared_ptr<T>, FT, node_catalog&)
    -> bound_function<T, FT>;
}

#endif /* meld_core_bound_function_hpp */
