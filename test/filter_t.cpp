#include "meld/graph/dynamic_join_node.hpp"
#include "meld/graph/serial_node.hpp"
#include "meld/utilities/debug.hpp"
#include "meld/utilities/thread_counter.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb/flow_graph.h"

using namespace meld;
using namespace oneapi::tbb;

namespace meld {

  enum class state : std::size_t { fail = 0, pass };

  template <typename Input>
  using data_msg_base = flow::tagged_msg<std::underlying_type_t<state>, bool, Input>;

  template <typename Input>
  class data_msg {
  public:
    data_msg() : data_msg{0u} {}
    explicit data_msg(unsigned int const msg_number, Input const& input) :
      msg_{static_cast<std::underlying_type_t<state>>(state::pass), input}, msg_number_{msg_number}
    {
    }

    auto
    pass(Input const& t) const
    {
      return data_msg<Input>{msg_number_, t};
    }
    auto
    fail() const
    {
      return data_msg<Input>{msg_number_};
    }

    unsigned
    msg_id() const
    {
      return msg_number_;
    }

    // Necessary API for tagged_msg
    template <typename T>
    auto&
    cast_to() const
    {
      return msg_.template cast_to<T>();
    }
    auto
    tag() const
    {
      return msg_.tag();
    }

  private:
    explicit data_msg(unsigned int const msg_number) :
      msg_{static_cast<std::underlying_type_t<state>>(state::fail), false}, msg_number_{msg_number}
    {
    }

    data_msg_base<Input> msg_;
    unsigned msg_number_;
  };

  template <typename Input>
  using filter_node_base = tbb::flow::function_node<Input, Input>;

  template <typename Input>
  class filter_node : public filter_node_base<data_msg<Input>> {
  public:
    template <typename FT>
    filter_node(tbb::flow::graph& g, std::size_t concurrency, FT ft) :
      filter_node_base<data_msg<Input>>{
        g, concurrency, [filter = std::move(ft)](data_msg<Input> const& input) {
          if (input.tag() == 0) {
            return input.fail();
          }
          if (auto res = filter(input.template cast_to<Input>())) {
            return input.pass(*res);
          }
          return input.fail();
        }}
    {
    }
  };

  template <typename Input>
  using producer_node_base = tbb::flow::function_node<Input, Input>;

  template <typename Input>
  class producer_node : public producer_node_base<data_msg<Input>> {
  public:
    template <typename FT>
    producer_node(tbb::flow::graph& g, std::size_t concurrency, FT ft) :
      producer_node_base<data_msg<Input>>{
        g, concurrency, [produce = std::move(ft)](data_msg<Input> const& input) {
          if (input.tag() == 0) {
            return input.fail();
          }
          return input.pass(produce(input.template cast_to<Input>()));
        }}
    {
    }
  };

  using data_msg_t = data_msg<unsigned int>;
}

TEST_CASE("Filter node + continue node", "[multithreading]")
{
  flow::graph g;
  flow::input_node src{g, [i = 0u](flow_control& fc) mutable -> data_msg_t {
                         if (i < 10u) {
                           return data_msg_t{i, ++i};
                         }
                         fc.stop();
                         return {};
                       }};

  filter_node<unsigned int> filter{
    g, flow::unlimited, [](unsigned int const i) -> std::optional<unsigned int> {
      debug("Filtering ", i);
      if (i % 2 != 0) {
        return std::nullopt;
      }
      return std::make_optional(i);
    }};

  producer_node<unsigned int> producer1{
    g, flow::unlimited, [](unsigned int const i) -> unsigned int {
      debug("Producer 1: ", i);
      return i;
    }};

  dynamic_join_node synchronize{g, [](data_msg_t const& msg) { return msg.msg_id(); }};
  producer_node<unsigned int> producer2{
    g, flow::unlimited, [](unsigned int const i) -> unsigned int {
      debug("Producer 2: ", i);
      return i;
    }};

  nodes(src)->nodes(filter, producer1)->nodes(synchronize)->nodes(producer2);

  src.activate();
  g.wait_for_all();
}
