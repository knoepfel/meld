#ifndef meld_core_common_node_options_hpp
#define meld_core_common_node_options_hpp

// =======================================================================================
// The facilities provided here will become simpler whenever "deducing this" is available
// in C++23 (which will largely replace CRTP).
// =======================================================================================

#include "meld/concurrency.hpp"
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
  concept input_argument = std::convertible_to<T, std::string> || does_specify_value<T>;

  // FIXME: Temporary API to tell the framework to use the given value for the specified
  //        argument instead of requiring a message.
  template <typename T>
  specified_value<T> use(T t)
  {
    return {std::move(t)};
  }

  template <typename T>
  class common_node_options {
  public:
    explicit common_node_options(T* t, boost::json::object const* config) : t_{t}
    {
      if (!config) {
        return;
      }
      if (auto concurrency = config->if_contains("concurrency")) {
        concurrency_ = concurrency->get_uint64();
      }
      if (auto preceding_filters = config->if_contains("filtered_by")) {
        preceding_filters_ = value_to<std::vector<std::string>>(*preceding_filters);
      }
    }

    T& concurrency(std::size_t n)
    {
      if (!concurrency_) {
        concurrency_ = n;
      }
      return *t_;
    }

    T& filtered_by(std::vector<std::string> preceding_filters)
    {
      if (!preceding_filters_) {
        preceding_filters_ = move(preceding_filters);
      }
      return *t_;
    }

    T& filtered_by(std::convertible_to<std::string> auto&&... names)
    {
      return filtered_by({std::forward<decltype(names)>(names)...});
    }

    decltype(auto) input(input_argument auto&&... ts)
    {
      static_assert(T::N == sizeof...(ts),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      return t_->input(std::make_tuple(std::forward<decltype(ts)>(ts)...));
    }

    std::vector<std::string> release_preceding_filters()
    {
      if (preceding_filters_) {
        return std::move(*preceding_filters_);
      }
      return {};
    }
    std::size_t concurrency() const noexcept { return concurrency_.value_or(concurrency::serial); }

  private:
    T* t_;
    std::optional<std::vector<std::string>> preceding_filters_{};
    std::optional<size_t> concurrency_{};
  };
}

#endif /* meld_core_common_node_options_hpp */
