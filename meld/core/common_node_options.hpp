#ifndef meld_core_common_node_options_hpp
#define meld_core_common_node_options_hpp

// =======================================================================================
// The facilities provided here will become simpler whenever "deducing this" is available
// in C++23 (which will largely replace CRTP).
// =======================================================================================

#include "meld/concurrency.hpp"
#include "meld/configuration.hpp"
#include "meld/core/concepts.hpp"
#include "meld/core/input_arguments.hpp"

#include "boost/json.hpp"

#include <concepts>
#include <optional>
#include <string>
#include <vector>

namespace meld {
  // FIXME: Need to support Boost JSON strings
  template <typename T>
  concept input_argument = std::convertible_to<T, specified_label>;

  template <typename T>
  class common_node_options {
  public:
    T& concurrency(std::size_t n)
    {
      if (!concurrency_) {
        concurrency_ = n;
      }
      return self();
    }

    T& filtered_by(std::vector<std::string> preceding_filters)
    {
      if (!preceding_filters_) {
        preceding_filters_ = std::move(preceding_filters);
      }
      return self();
    }

    T& filtered_by(std::convertible_to<std::string> auto&&... names)
    {
      return filtered_by({std::forward<decltype(names)>(names)...});
    }

    decltype(auto) react_to(std::convertible_to<std::string> auto&&... ts)
    {
      return input(specified_label{std::forward<decltype(ts)>(ts)}...);
    }

    decltype(auto) input(input_argument auto&&... ts)
    {
      static_assert(T::N == sizeof...(ts),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      return self().input({specified_label{std::forward<decltype(ts)>(ts)}...});
    }

  protected:
    explicit common_node_options(configuration const* config)
    {
      if (!config) {
        return;
      }
      concurrency_ = config->get_if_present<int>("concurrency");
      preceding_filters_ = config->get_if_present<std::vector<std::string>>("filtered_by");
    }

    // N.B. For the below functions, std::move is used to take advantage of the
    //      &&-qualified value_or optimization, described at
    //      https://en.cppreference.com/w/cpp/utility/optional/value_or

    std::vector<std::string> release_store_names()
    {
      return std::move(store_names_).value_or(std::vector<std::string>{});
    }

    std::vector<std::string> release_preceding_filters()
    {
      return std::move(preceding_filters_).value_or(std::vector<std::string>{});
    }
    std::size_t concurrency() const noexcept { return concurrency_.value_or(concurrency::serial); }

  private:
    auto& self() { return *static_cast<T*>(this); }
    std::optional<std::vector<std::string>> store_names_{};
    std::optional<std::vector<std::string>> preceding_filters_{};
    std::optional<size_t> concurrency_{};
  };
}

#endif /* meld_core_common_node_options_hpp */
