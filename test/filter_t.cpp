#include "meld/graph/make_edges.hpp"
#include "meld/utilities/debug.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/flow_graph.h"

#include <cassert>

using namespace meld;
using namespace oneapi::tbb;

namespace {
  struct filter_result {
    unsigned int msg_id;
    bool result;
  };

  constexpr unsigned int true_value{-1u};
  constexpr unsigned int false_value{-2u};

  constexpr bool is_complete(unsigned int value)
  {
    return value == false_value or value == true_value;
  }

  constexpr bool to_boolean(unsigned int value)
  {
    assert(is_complete(value));
    return value == true_value;
  }

  template <typename T>
  class data_map {
  public:
    explicit data_map(unsigned int const nargs) : nargs_{nargs} {}

    void update(T const& t)
    {
      typename decltype(data_)::accessor a;
      if (data_.insert(a, t)) { // data serving as proxy for msg_id
        a->second.reserve(nargs_);
      }
      a->second.push_back(t);
    }

    bool is_complete(unsigned int msg_id) const
    {
      typename decltype(data_)::const_accessor a;
      if (data_.find(a, msg_id)) {
        return a->second.size() == nargs_;
      }
      return false;
    }

    std::vector<T> release_data(unsigned int msg_id)
    {
      typename decltype(data_)::accessor a;
      bool const rc [[maybe_unused]] = data_.find(a, msg_id);
      assert(rc);
      return std::move(a->second);
    }

    void erase(unsigned int msg_id) { data_.erase(msg_id); }

  private:
    std::size_t nargs_;
    concurrent_hash_map<unsigned int, std::vector<T>> data_;
  };

  class decision_map {
  public:
    explicit decision_map(unsigned int total_decisions) : total_decisions_{total_decisions} {}

    void update(filter_result result)
    {
      decltype(results_)::accessor a;
      results_.insert(a, result.msg_id);

      // First check that the decision is not already complete
      if (is_complete(a->second)) {
        return;
      }

      if (not result.result) {
        a->second = false_value;
        return;
      }

      ++a->second;

      if (a->second == total_decisions_) {
        a->second = true_value;
      }
    }

    unsigned int value(unsigned int msg_id) const
    {
      decltype(results_)::const_accessor a;
      if (results_.find(a, msg_id)) {
        return a->second;
      }
      return 0u;
    }

    void erase(unsigned int msg_id) { results_.erase(msg_id); }

  private:
    unsigned int const total_decisions_;
    concurrent_hash_map<unsigned int, unsigned int> results_;
  };

  template <typename T>
  using filter_node_base = flow::function_node<T, filter_result>;

  template <typename T>
  class filter_node : public filter_node_base<T> {
  public:
    template <typename FT>
    filter_node(flow::graph& g, std::size_t concurrency, FT ft) :
      filter_node_base<T>{g, concurrency, [f = std::move(ft)](T const& t) -> filter_result {
                            return {.msg_id = t, .result = f(t)};
                          }}
    {
    }
  };

  template <typename T>
  using sink_node_base = flow::function_node<T, flow::continue_msg>;
  template <typename T>
  class sink_node : public sink_node_base<T> {
  public:
    template <typename FT>
    sink_node(tbb::flow::graph& g, std::size_t concurrency, FT ft) :
      sink_node_base<T>{g, concurrency, [f = std::move(ft)](T const& t) -> flow::continue_msg {
                          f(t);
                          return {};
                        }}
    {
    }
  };

  template <typename T>
  using aggregator = flow::composite_node<std::tuple<filter_result, T>, std::tuple<T>>;

  template <typename T>
  class filter_aggregator : public aggregator<T> {
    using indexer_t = flow::indexer_node<filter_result, T>;
    using tag_t = typename indexer_t::output_type;

  public:
    using typename aggregator<T>::input_ports_type;
    using typename aggregator<T>::output_ports_type;

    explicit filter_aggregator(flow::graph& g, unsigned int ndecisions, unsigned nargs) :
      aggregator<T>{g},
      decisions_{ndecisions},
      data_{nargs},
      indexer_{g},
      filter_{g, flow::unlimited, [this](tag_t const& t, auto& outputs) {
                unsigned int msg_id{};
                if (t.template is_a<T>()) {
                  msg_id = t.template cast_to<T>();
                  data_.update(msg_id); // msg_id serves as proxy for data
                }
                else {
                  auto const& result = t.template cast_to<filter_result>();
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
                  auto const data = data_.release_data(msg_id);
                  for (auto const& d : data) {
                    out.try_put(d);
                  }
                  data_.erase(msg_id);
                  decisions_.erase(msg_id);
                }
              }}
    {
      make_edge(indexer_, filter_);
      aggregator<T>::set_external_ports(
        input_ports_type{input_port<0>(indexer_), input_port<1>(indexer_)},
        output_ports_type{output_port<0>(filter_)});
    }

  private:
    decision_map decisions_;
    data_map<T> data_;
    indexer_t indexer_;
    flow::multifunction_node<tag_t, std::tuple<T>> filter_;
  };

  template <typename T>
  class source {
  public:
    explicit source(T const max_n) : max_{max_n} {}
    unsigned int operator()(flow_control& fc)
    {
      if (i_ < max_) {
        return i_++;
      }
      fc.stop();
      return {};
    }

  private:
    T const max_;
    T i_{};
  };
}

TEST_CASE("Filter decision", "[filtering]")
{
  decision_map decisions{2};
  decisions.update({1, false});
  {
    auto const value = decisions.value(1);
    CHECK(is_complete(value));
    CHECK(to_boolean(value) == false);
    decisions.erase(1);
  }
  decisions.update({3, true});
  {
    auto const value = decisions.value(3);
    CHECK(not is_complete(value));
  }
  decisions.update({3, true});
  {
    auto const value = decisions.value(3);
    CHECK(is_complete(value));
    CHECK(to_boolean(value) == true);
    decisions.erase(3);
  }
}

TEST_CASE("One filter", "[filtering]")
{
  flow::graph g;
  flow::input_node src{g, source{10u}};

  filter_node<unsigned int> filter{
    g, flow::unlimited, [](unsigned int const i) -> bool { return i % 2 == 0; }};

  filter_aggregator<unsigned int> collector{g, 1u, 1u};

  std::atomic<unsigned int> sum{};
  sink_node<unsigned int> add{g, flow::unlimited, [&sum](unsigned int const i) { sum += i; }};

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

  filter_node<unsigned int> filter_evens{
    g, flow::unlimited, [](unsigned int const i) -> bool { return i % 2 == 0; }};

  filter_node<unsigned int> filter_odds{
    g, flow::unlimited, [](unsigned int const i) -> bool { return i % 2 != 0; }};

  filter_aggregator<unsigned int> collector_evens{g, 1u, 1u};
  filter_aggregator<unsigned int> collector_odds{g, 1u, 1u};

  std::atomic<unsigned int> sum{};
  sink_node<unsigned int> add{g, flow::unlimited, [&sum](unsigned int const i) { sum += i; }};

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

  filter_node<unsigned int> filter_evens{
    g, flow::unlimited, [](unsigned int const i) -> bool { return i % 2 == 0; }};

  filter_node<unsigned int> filter_odds{
    g, flow::unlimited, [](unsigned int const i) -> bool { return i % 2 != 0; }};

  filter_aggregator<unsigned int> collector{g, 2u, 1u};

  std::atomic<unsigned int> sum{};
  sink_node<unsigned int> add{g, flow::unlimited, [&sum](unsigned int const i) { sum += i; }};

  nodes(src)->nodes(filter_evens, filter_odds, input_port<1>(collector));
  nodes(filter_evens, filter_odds)->nodes(input_port<0>(collector));
  nodes(output_port<0>(collector))->nodes(add);

  src.activate();
  g.wait_for_all();

  CHECK(sum == 0u);
}
