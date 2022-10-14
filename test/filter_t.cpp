#include "meld/core/filter/filter_impl.hpp"
#include "meld/core/message.hpp"
#include "meld/core/product_store.hpp"
#include "meld/graph/make_edges.hpp"
#include "meld/utilities/debug.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/concurrent_vector.h"
#include "oneapi/tbb/flow_graph.h"

using namespace meld;
using namespace oneapi::tbb;

namespace {
  using filter_node_base = flow::function_node<message, filter_result>;

  class filter_node : public filter_node_base {
  public:
    template <typename FT>
    filter_node(flow::graph& g, std::size_t concurrency, FT ft) :
      filter_node_base{g, concurrency, [f = std::move(ft)](message const& msg) -> filter_result {
                         return {.msg_id = msg.id, .result = f(msg.store)};
                       }}
    {
    }
  };

  using sink_node_base = flow::function_node<message, flow::continue_msg>;
  class sink_node : public sink_node_base {
  public:
    template <typename FT>
    sink_node(tbb::flow::graph& g, std::size_t concurrency, FT ft) :
      sink_node_base{g, concurrency, [f = std::move(ft)](message const& msg) -> flow::continue_msg {
                       f(msg.store);
                       return {};
                     }}
    {
    }
  };

  using aggregator = flow::composite_node<std::tuple<filter_result, message>, messages_t<1>>;

  class filter_aggregator : public aggregator {
    using indexer_t = flow::indexer_node<filter_result, message>;
    using tag_t = typename indexer_t::output_type;

  public:
    using typename aggregator::input_ports_type;
    using typename aggregator::output_ports_type;

    explicit filter_aggregator(flow::graph& g, unsigned int ndecisions, unsigned nargs) :
      aggregator{g},
      decisions_{ndecisions},
      data_{nargs},
      indexer_{g},
      filter_{g, flow::unlimited, [this](tag_t const& t, auto& outputs) {
                unsigned int msg_id{};
                if (t.is_a<message>()) {
                  auto const& msg = t.cast_to<message>();
                  data_.update(msg.id, msg.store);
                  msg_id = msg.id;
                }
                else {
                  auto const& result = t.cast_to<filter_result>();
                  decisions_.update(result);
                  msg_id = result.msg_id;
                }

                auto const filter_decision = decisions_.value(msg_id);
                if (not is_complete(filter_decision)) {
                  return;
                }

                if (not data_.is_complete(msg_id)) {
                  return;
                }

                if (to_boolean(filter_decision)) {
                  auto& out = std::get<0>(outputs);
                  auto const stores = data_.release_data(msg_id);
                  for (auto const& d : stores) {
                    out.try_put({d, msg_id});
                  }
                  data_.erase(msg_id);
                  decisions_.erase(msg_id);
                }
              }}
    {
      make_edge(indexer_, filter_);
      aggregator::set_external_ports(
        input_ports_type{input_port<0>(indexer_), input_port<1>(indexer_)},
        output_ports_type{output_port<0>(filter_)});
    }

  private:
    decision_map decisions_;
    data_map data_;
    indexer_t indexer_;
    flow::multifunction_node<tag_t, messages_t<1>> filter_;
  };

  class source {
  public:
    explicit source(unsigned const max_n) : max_{max_n} {}
    message operator()(flow_control& fc)
    {
      if (i_ < max_) {
        std::size_t const msg_id = i_;
        auto store = make_product_store(level_id{i_});
        store->add_product<unsigned int>("num", i_++);
        return {move(store), msg_id};
      }
      fc.stop();
      return {};
    }

  private:
    unsigned const max_;
    unsigned i_{};
  };

  constexpr auto evens_only()
  {
    return [](product_store_ptr const& store) -> bool {
      return store->get_product<unsigned int>("num") % 2 == 0;
    };
  }
  constexpr auto odds_only()
  {
    return [](product_store_ptr const& store) -> bool {
      return store->get_product<unsigned int>("num") % 2 != 0;
    };
  }

  constexpr auto sum_numbers(std::atomic<unsigned int>& sum)
  {
    return
      [&sum](product_store_ptr const& store) { sum += store->get_product<unsigned int>("num"); };
  }

  constexpr bool in_range(unsigned int const b, unsigned int const e, unsigned int const i)
  {
    return i >= b and i < e;
  }

  constexpr auto not_in_range(unsigned int const b, unsigned int const e)
  {
    return [b, e](product_store_ptr const& store) -> bool {
      return not in_range(b, e, store->get_product<unsigned int>("num"));
    };
  }
}

TEST_CASE("One filter", "[filtering]")
{
  flow::graph g;
  flow::input_node src{g, source{10u}};

  filter_node filter{g, flow::unlimited, evens_only()};
  filter_aggregator collector{g, 1u, 1u};

  std::atomic<unsigned int> sum{};
  sink_node add{g, flow::unlimited, sum_numbers(sum)};

  nodes(src)->nodes(filter, input_port<1>(collector));
  nodes(filter)->nodes(input_port<0>(collector));
  nodes(output_port<0>(collector))->nodes(add);

  src.activate();
  g.wait_for_all();

  CHECK(sum == 20u);
}

TEST_CASE("Two filters in series", "[filtering]")
{
  flow::graph g;
  flow::input_node src{g, source{10u}};

  filter_node filter_evens{g, flow::unlimited, evens_only()};
  filter_node filter_odds{g, flow::unlimited, odds_only()};

  filter_aggregator collector_evens{g, 1u, 1u};
  filter_aggregator collector_odds{g, 1u, 1u};

  std::atomic<unsigned int> sum{};
  sink_node add{g, flow::unlimited, sum_numbers(sum)};

  SECTION("Filter evens then odds")
  {
    nodes(src)->nodes(filter_evens, input_port<1>(collector_evens));
    nodes(filter_evens)->nodes(input_port<0>(collector_evens));
    nodes(output_port<0>(collector_evens))->nodes(filter_odds, input_port<1>(collector_odds));
    nodes(filter_odds)->nodes(input_port<0>(collector_odds));
    nodes(output_port<0>(collector_odds))->nodes(add);
  }

  SECTION("Filter odds then evens")
  {
    nodes(src)->nodes(filter_odds, input_port<1>(collector_odds));
    nodes(filter_odds)->nodes(input_port<0>(collector_odds));
    nodes(output_port<0>(collector_odds))->nodes(filter_evens, input_port<1>(collector_evens));
    nodes(filter_evens)->nodes(input_port<0>(collector_evens));
    nodes(output_port<0>(collector_evens))->nodes(add);
  }

  // The following is invoked for *each* section above
  src.activate();
  g.wait_for_all();

  CHECK(sum == 0u);
}

TEST_CASE("Two filters in parallel", "[filtering]")
{
  flow::graph g;
  flow::input_node src{g, source{10u}};

  filter_node filter_evens{g, flow::unlimited, evens_only()};
  filter_node filter_odds{g, flow::unlimited, odds_only()};

  filter_aggregator collector{g, 2u, 1u};

  std::atomic<unsigned int> sum{};
  sink_node add{g, flow::unlimited, sum_numbers(sum)};

  nodes(src)->nodes(filter_evens, filter_odds, input_port<1>(collector));
  nodes(filter_evens, filter_odds)->nodes(input_port<0>(collector));
  nodes(output_port<0>(collector))->nodes(add);

  src.activate();
  g.wait_for_all();

  CHECK(sum == 0u);
}

TEST_CASE("Three filters in parallel", "[filtering]")
{
  flow::graph g;
  flow::input_node src{g, source{10u}};

  filter_node exclude_0_to_4{g, flow::unlimited, not_in_range(0, 4)};
  filter_node exclude_6_to_7{g, flow::unlimited, not_in_range(6, 7)};
  filter_node exclude_gt_8{g, flow::unlimited, not_in_range(8, -1u)};

  filter_aggregator collector{g, 3u, 1u};

  concurrent_vector<unsigned int> nums;
  sink_node add{g, flow::unlimited, [&nums](product_store_ptr const& store) {
                  nums.push_back(store->get_product<unsigned int>("num"));
                }};

  nodes(src)->nodes(exclude_0_to_4, exclude_6_to_7, exclude_gt_8, input_port<1>(collector));
  nodes(exclude_0_to_4, exclude_6_to_7, exclude_gt_8)->nodes(input_port<0>(collector));
  nodes(output_port<0>(collector))->nodes(add);

  src.activate();
  g.wait_for_all();

  std::vector actual(std::begin(nums), std::end(nums));
  sort(begin(actual), end(actual));
  std::vector const expected{4u, 5u, 7u};
  CHECK(actual == expected);
}
