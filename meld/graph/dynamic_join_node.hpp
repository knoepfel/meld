#ifndef meld_graph_dynamic_join_node_hpp
#define meld_graph_dynamic_join_node_hpp

#include "oneapi/tbb/flow_graph.h"

#include <cassert>

namespace meld {
  namespace detail {
    template <typename Input>
    using join_and_reduce_base =
      tbb::flow::composite_node<std::tuple<Input, Input>, std::tuple<Input>>;

    template <typename Input>
    class join_and_reduce : public join_and_reduce_base<Input> {
    public:
      template <typename TagMatcher>
      explicit join_and_reduce(tbb::flow::graph& g, TagMatcher& ft) :
        join_and_reduce_base<Input>{g},
        join_{g, ft, ft},
        reduce_{g, tbb::flow::unlimited, [](auto& pr) { return get<0>(pr); }}

      {
        make_edge(join_, reduce_);
        join_and_reduce_base<Input>::set_external_ports(
          typename join_and_reduce_base<Input>::input_ports_type{input_port<0>(join_),
                                                                 input_port<1>(join_)},
          typename join_and_reduce_base<Input>::output_ports_type{reduce_});
      }

    private:
      tbb::flow::join_node<std::tuple<Input, Input>, tbb::flow::tag_matching> join_;
      tbb::flow::function_node<std::tuple<Input, Input>, Input> reduce_;
    };
  }

  template <typename Input, typename TagMatcher>
  class dynamic_join_node {
  public:
    template <typename FuncType>
    explicit dynamic_join_node(tbb::flow::graph& g, FuncType&& tag_matcher) :
      graph_{g}, tag_matcher_{tag_matcher}
    {
    }

  private:
    template <typename In, typename Output, typename Matcher>
    friend void make_edge(tbb::flow::sender<In>& input, dynamic_join_node<Output, Matcher>& djoin);

    template <typename In, typename Matcher, typename Output>
    friend void make_edge(dynamic_join_node<In, Matcher>& djoin,
                          tbb::flow::receiver<Output>& output);

    template <typename T>
    void output_edge(tbb::flow::receiver<T>& rec)
    {
      if (empty(senders_)) {
        return;
      }
      if (empty(joins_)) {
        assert(size(senders_) == 1ul);
        make_edge(*senders_[0], rec);
      }
      else {
        make_edge(*joins_.back(), rec);
      }
    }

    void input_edge(tbb::flow::sender<Input>& sender)
    {
      senders_.push_back(&sender);
      auto const n_senders = size(senders_);

      if (n_senders == 1u) {
        return;
      }

      joins_.push_back(std::make_shared<detail::join_and_reduce<Input>>(graph_, tag_matcher_));

      auto const current_index = size(joins_) - 1;
      auto& current_join_node = *joins_[current_index];

      if (n_senders == 2u) {
        make_edge(*senders_[0], input_port<0>(current_join_node));
      }
      else {
        auto& previous_join_node = *joins_[current_index - 1];
        make_edge(previous_join_node, input_port<0>(current_join_node));
      }
      make_edge(sender, input_port<1>(current_join_node));
    }

  private:
    tbb::flow::graph& graph_;
    TagMatcher tag_matcher_;
    std::vector<tbb::flow::sender<Input>*> senders_;
    std::vector<std::shared_ptr<detail::join_and_reduce<Input>>> joins_;
  };

  template <typename Input, typename Output, typename TagMatcher>
  void make_edge(tbb::flow::sender<Input>& input, dynamic_join_node<Output, TagMatcher>& djoin)
  {
    djoin.input_edge(input);
  }

  template <typename Input, typename TagMatcher, typename Output>
  void make_edge(dynamic_join_node<Input, TagMatcher>& djoin, tbb::flow::receiver<Output>& output)
  {
    djoin.output_edge(output);
  }

  namespace detail {
    template <typename TagMatcher, typename R, typename T>
    T input_for(R (TagMatcher::*)(T) const);

    template <typename TagMatcher, typename R, typename T>
    T input_for(R (TagMatcher::*)(T));

    template <typename TagMatcher, typename R, typename T>
    std::remove_const_t<T> input_for(R (TagMatcher::*)(T&) const);

    template <typename TagMatcher, typename R, typename T>
    std::remove_const_t<T> input_for(R (TagMatcher::*)(T&));

    template <typename TagMatcher>
    using input_for_t = decltype(input_for(&TagMatcher::operator()));
  }

  template <typename Graph, typename TagMatcher>
  dynamic_join_node(Graph&, TagMatcher&&)
    -> dynamic_join_node<detail::input_for_t<TagMatcher>, TagMatcher>;
}

#endif /* meld_graph_dynamic_join_node_hpp */
