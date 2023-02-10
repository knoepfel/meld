#ifndef meld_core_input_arguments_hpp
#define meld_core_input_arguments_hpp

#include "meld/core/message.hpp"

#include <cstddef>
#include <string>
#include <tuple>
#include <utility>

namespace meld {
  template <typename T, std::size_t JoinNodePort>
  struct expects_message {
    using handle_arg_t = typename handle_for<T>::value_type;
    std::string name;
    auto retrieve(auto const& messages) const
    {
      return std::get<JoinNodePort>(messages).store->template get_handle<handle_arg_t>(name);
    }
  };

  struct specified_label {
    std::string name;
  };

  // =====================================================================================

  template <typename T>
  constexpr bool does_expect_message = false;

  template <typename T, std::size_t I>
  constexpr bool does_expect_message<expects_message<T, I>> = true;
}

#endif /* meld_core_input_arguments_hpp */
