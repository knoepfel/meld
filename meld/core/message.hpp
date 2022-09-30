#ifndef meld_core_message_hpp
#define meld_core_message_hpp

#include "meld/core/product_store.hpp"
#include "meld/utilities/sized_tuple.hpp"

#include "oneapi/tbb/flow_graph.h" // <-- belongs somewhere else

#include <cstddef>
#include <span>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace meld {

  struct message {
    product_store_ptr store;
    std::size_t id;
    std::size_t original_id{-1ull}; // Used during flush
  };

  template <std::size_t N>
  using messages_t = sized_tuple<message, N>;

  struct MessageHasher {
    std::size_t operator()(message const& msg) const noexcept;
  };

  // Overload for use with most_derived
  message const& more_derived(message const& a, message const& b);

  namespace detail {
    template <std::size_t N>
    using join_messages_t = tbb::flow::join_node<messages_t<N>, tbb::flow::tag_matching>;
    using no_join_base_t = tbb::flow::function_node<message, messages_t<1ull>>;

    struct no_join : no_join_base_t {
      no_join(tbb::flow::graph& g, MessageHasher) :
        no_join_base_t{g, tbb::flow::unlimited, [](message const& msg) { return std::tuple{msg}; }}
      {
      }
    };
  }

  template <std::size_t N>
  using join_or_none_t = std::conditional_t<N == 1ull, detail::no_join, detail::join_messages_t<N>>;

  std::size_t port_index_for(std::span<std::string const> product_names,
                             std::string const& product_name);

  template <std::size_t I, std::size_t N>
  tbb::flow::receiver<message>& receiver_for(detail::join_messages_t<N>& join,
                                             std::size_t const index)
  {
    if constexpr (I < N) {
      if (I != index) {
        return receiver_for<I + 1ull, N>(join, index);
      }
      return input_port<I>(join);
    }
    throw std::runtime_error("Should never get here");
  }

  template <std::size_t N>
  tbb::flow::receiver<message>& receiver_for(join_or_none_t<N>& join,
                                             std::span<std::string const> product_names,
                                             std::string const& product_name)
  {
    if constexpr (N > 1ull) {
      auto const index = port_index_for(product_names, product_name);
      return receiver_for<0ull, N>(join, index);
    }
    else {
      return join;
    }
  }

  template <std::size_t I, std::size_t N, typename U>
  auto get_handle_for(messages_t<N> const& messages, std::span<std::string const> product_names)
  {
    using handle_arg_t = typename handle_for<U>::value_type;
    return std::get<I>(messages).store->template get_handle<handle_arg_t>(product_names[I]);
  }

}

#endif /* meld_core_message_hpp */
