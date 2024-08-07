#ifndef meld_core_input_arguments_hpp
#define meld_core_input_arguments_hpp

#include "meld/core/message.hpp"
#include "meld/core/specified_label.hpp"

#include "fmt/format.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <set>
#include <string>
#include <tuple>
#include <utility>

namespace meld {
  template <typename T, std::size_t JoinNodePort>
  struct retriever {
    using handle_arg_t = typename handle_for<T>::value_type;
    specified_label label;
    auto retrieve(auto const& messages) const
    {
      return std::get<JoinNodePort>(messages).store->template get_handle<handle_arg_t>(
        label.name.full());
    }
  };

  template <typename InputTypes, std::size_t... Is>
  auto form_input_arguments_impl(std::array<specified_label, sizeof...(Is)> args,
                                 std::index_sequence<Is...>)
  {
    return std::make_tuple(
      retriever<std::tuple_element_t<Is, InputTypes>, Is>{std::move(args[Is])}...);
  }

  // =====================================================================================

  template <typename InputTypes, std::size_t N>
  auto form_input_arguments(std::string const& algorithm_name, std::array<specified_label, N> args)
  {
    auto sorted = args;
    std::sort(begin(sorted), end(sorted));
    std::set unique_and_sorted(begin(sorted), end(sorted));
    std::vector<specified_label> duplicates;
    std::set_difference(begin(sorted),
                        end(sorted),
                        begin(unique_and_sorted),
                        end(unique_and_sorted),
                        std::back_inserter(duplicates));
    if (not empty(duplicates)) {
      std::string error =
        fmt::format("Algorithm '{}' uses the following products more than once:\n", algorithm_name);
      for (auto const& label : duplicates) {
        error += fmt::format(" - '{}'\n", label.to_string());
      }
      throw std::runtime_error(error);
    }

    return form_input_arguments_impl<InputTypes>(std::move(args), std::make_index_sequence<N>{});
  }
}

#endif /* meld_core_input_arguments_hpp */
