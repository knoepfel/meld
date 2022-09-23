#ifndef meld_core_message_hpp
#define meld_core_message_hpp

#include "meld/core/product_store.hpp"
#include "meld/utilities/sized_tuple.hpp"

#include "oneapi/tbb/flow_graph.h" // <-- belongs somewhere else

#include <cstddef>
#include <tuple>
#include <type_traits>

namespace meld {

  struct message {
    product_store_ptr store;
    std::size_t id;
    std::size_t original_id; // Used during flush
  };

  template <std::size_t N>
  using messages_t = sized_tuple<message, N>;

  struct MessageHasher {
    std::size_t operator()(message const& msg) const noexcept;
  };

  // Overload for use with most_derived
  message const& more_derived(message const& a, message const& b);

  inline namespace put_somplace_else {
    template <std::size_t N>
    using join_messages_t = tbb::flow::join_node<messages_t<N>, tbb::flow::tag_matching>;

    struct no_join {
      no_join(tbb::flow::graph& g, MessageHasher) :
        pass_through{g, tbb::flow::unlimited, [](message const& msg) { return std::tuple{msg}; }}
      {
      }
      tbb::flow::function_node<message, messages_t<1ull>> pass_through;
    };
  }

  template <std::size_t N>
  using join_or_none_t = std::conditional_t<N == 1ull, no_join, join_messages_t<N>>;
}

#endif /* meld_core_message_hpp */
