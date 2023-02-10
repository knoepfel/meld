#ifndef meld_core_bound_function_hpp
#define meld_core_bound_function_hpp

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

  template <typename T, typename FT>
  class bound_function : public node_options<bound_function<T, FT>> {
    using node_options_t = node_options<bound_function<T, FT>>;
    using input_parameter_types = parameter_types<FT>;

  public:
    static constexpr auto N = number_parameters<FT>;

    bound_function(configuration const* config,
                   std::string name,
                   std::shared_ptr<T> obj,
                   FT f,
                   tbb::flow::graph& g,
                   node_catalog& nodes,
                   std::vector<std::string>& errors) :
      node_options_t{config},
      name_{std::move(name)},
      obj_{obj},
      ft_{std::move(f)},
      graph_{g},
      nodes_{nodes},
      errors_{errors}
    {
    }

    template <typename... InputArgs>
    auto filter(std::tuple<InputArgs...> input_args)
      requires is_filter_like<FT>
    {
      static_assert(N == sizeof...(InputArgs),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      auto processed_input_args =
        detail::form_input_arguments<input_parameter_types>(std::move(input_args));
      nodes_.register_filter(errors_).set([this, inputs = std::move(processed_input_args)] {
        auto product_names = detail::port_names(inputs);
        auto function_closure = delegate(obj_, ft_);

        return std::make_unique<
          complete_filter<decltype(function_closure), decltype(processed_input_args)>>(
          std::move(name_),
          node_options_t::concurrency(),
          node_options_t::release_preceding_filters(),
          std::vector<std::string>{},
          graph_,
          std::move(function_closure),
          std::move(inputs),
          std::move(product_names));
      });
    }

    template <typename... InputArgs>
    auto monitor(std::tuple<InputArgs...> input_args)
      requires is_monitor_like<FT>
    {
      static_assert(N == sizeof...(InputArgs),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      auto processed_input_args =
        detail::form_input_arguments<input_parameter_types>(std::move(input_args));
      nodes_.register_monitor(errors_).set([this, inputs = std::move(processed_input_args)] {
        auto product_names = detail::port_names(inputs);
        auto function_closure = delegate(obj_, ft_);

        return std::make_unique<
          complete_monitor<decltype(function_closure), decltype(processed_input_args)>>(
          std::move(name_),
          node_options_t::concurrency(),
          node_options_t::release_preceding_filters(),
          std::vector<std::string>{},
          graph_,
          std::move(function_closure),
          std::move(inputs),
          std::move(product_names));
      });
    }

    template <typename... InputArgs>
    auto reduce_all(std::tuple<InputArgs...> input_args [[maybe_unused]])
      requires is_reduction_like<FT>
    {
    }

    template <typename... InputArgs>
    auto split(std::tuple<InputArgs...> input_args)
      requires is_splitter_like<FT>
    {
      static_assert(N == sizeof...(InputArgs) + 1,
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      using all_but_first = skip_first_type<input_parameter_types>;
      auto processed_input_args =
        detail::form_input_arguments<all_but_first>(std::move(input_args));
      auto product_names = detail::port_names(processed_input_args);
      auto function_closure = delegate(obj_, ft_);

      return partial_splitter<decltype(function_closure), decltype(processed_input_args)>{
        nodes_.register_splitter(errors_),
        std::move(name_),
        node_options_t::concurrency(),
        node_options_t::release_preceding_filters(),
        std::vector<std::string>{},
        graph_,
        std::move(function_closure),
        std::move(processed_input_args),
        std::move(product_names)};
    }

    template <typename... InputArgs>
    auto transform(std::tuple<InputArgs...> input_args)
      requires is_transform_like<FT>
    {
      static_assert(N == sizeof...(InputArgs),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      auto processed_input_args =
        detail::form_input_arguments<input_parameter_types>(std::move(input_args));
      auto product_names = detail::port_names(processed_input_args);
      auto function_closure = delegate(obj_, ft_);

      return pre_transform<decltype(function_closure), decltype(processed_input_args)>{
        nodes_.register_transform(errors_),
        std::move(name_),
        node_options_t::concurrency(),
        node_options_t::release_preceding_filters(),
        graph_,
        std::move(function_closure),
        std::move(processed_input_args),
        std::move(product_names)};
    }

    auto filter(std::convertible_to<std::string> auto... input_args)
    {
      return filter(std::make_tuple(react_to(std::forward<decltype(input_args)>(input_args))...));
    }

    auto monitor(std::convertible_to<std::string> auto... input_args)
    {
      return monitor(std::make_tuple(react_to(std::forward<decltype(input_args)>(input_args))...));
    }

    auto split(std::convertible_to<std::string> auto... input_args)
    {
      return split(std::make_tuple(react_to(std::forward<decltype(input_args)>(input_args))...));
    }

    auto transform(std::convertible_to<std::string> auto... input_args)
    {
      return transform(
        std::make_tuple(react_to(std::forward<decltype(input_args)>(input_args))...));
    }

  private:
    std::string name_;
    std::shared_ptr<T> obj_;
    FT ft_;
    tbb::flow::graph& graph_;
    node_catalog& nodes_;
    std::vector<std::string>& errors_;
  };

  template <typename T, typename FT>
  bound_function(configuration const*, std::string, std::shared_ptr<T>, FT, node_catalog&)
    -> bound_function<T, FT>;
}

#endif /* meld_core_bound_function_hpp */
