#ifndef meld_graph_serial_node_hpp
#define meld_graph_serial_node_hpp

#include "meld/utilities/sized_tuple.hpp"
#include "oneapi/tbb/flow_graph.h"

namespace meld {

  using token_t = int;
  using base_impl = tbb::flow::buffer_node<token_t>;
  class serializer_node : public base_impl {
  public:
    explicit serializer_node(tbb::flow::graph& g) : base_impl{g} { try_put(1); }
  };

  template <typename T>
  concept is_serializer = std::same_as<T, serializer_node>;

  template <typename Input>
  using base = tbb::flow::composite_node<std::tuple<Input>, std::tuple<Input>>;

  namespace detail {
    template <typename Input, std::size_t N>
    using join_tuple = concatenated_tuples<std::tuple<Input>, sized_tuple<token_t, N>>;
  }

  template <typename Input, std::size_t N>
  class serial_node : public base<Input> {
    template <typename... Serializers, std::size_t... I>
    void
    make_edges(std::index_sequence<I...>, Serializers&... serializers)
    {
      (make_edge(serializers, input_port<I + 1>(join_)), ...);
    }

  public:
    template <typename FT, is_serializer... Serializers>
    explicit serial_node(tbb::flow::graph& g, FT f, Serializers&... serializers) :
      base<Input>{g},
      buffered_msgs_{g},
      join_{g},
      serialized_function_{g,
                           tbb::flow::serial,
                           [function = std::move(f),
                            &serializers...](detail::join_tuple<Input, N> const& tup) mutable {
                             auto input = std::get<0>(tup);
                             function(input);
                             (serializers.try_put(1), ...);
                             return input;
                           }}
    {
      make_edge(buffered_msgs_, input_port<0>(join_));
      make_edges(std::index_sequence_for<Serializers...>(), serializers...);
      make_edge(join_, serialized_function_);
      base<Input>::set_external_ports(
        typename base<Input>::input_ports_type{buffered_msgs_},
        typename base<Input>::output_ports_type{serialized_function_});
    }

  private:
    tbb::flow::buffer_node<Input> buffered_msgs_;
    tbb::flow::join_node<detail::join_tuple<Input, N>, tbb::flow::reserving> join_;
    tbb::flow::function_node<detail::join_tuple<Input, N>, Input> serialized_function_;
  };
}

#endif /* meld_graph_serial_node_hpp */
