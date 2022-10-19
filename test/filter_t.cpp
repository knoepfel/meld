#include "meld/core/filter/filter_impl.hpp"
#include "meld/core/filter/result_collector.hpp"
#include "meld/core/framework_graph.hpp"
#include "meld/core/message.hpp"
#include "meld/core/product_store.hpp"
#include "meld/graph/make_edges.hpp"
#include "meld/utilities/debug.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/concurrent_vector.h"
#include "oneapi/tbb/flow_graph.h"

using namespace meld;
using namespace meld::concurrency;
using namespace oneapi::tbb;

namespace {
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

  class source {
  public:
    explicit source(unsigned const max_n) : max_{max_n} {}
    product_store_ptr next()
    {
      if (i_ < max_) {
        auto store = make_product_store(level_id{i_});
        store->add_product<unsigned int>("num", i_);
        store->add_product<unsigned int>("other_num", 100 + i_);
        ++i_;
        return store;
      }
      return nullptr;
    }

  private:
    unsigned const max_;
    unsigned i_{};
  };

  constexpr bool evens_only(unsigned int const value) { return value % 2u == 0u; }
  constexpr bool odds_only(unsigned int const value) { return not evens_only(value); }

  // Hacky!
  struct sum_numbers {
    sum_numbers(unsigned int const n) : total{n} {}
    ~sum_numbers() { CHECK(sum == total); }
    void add(unsigned int const num) { sum += num; }
    std::atomic<unsigned int> sum;
    unsigned int const total;
  };

  // Hacky!
  struct collect_numbers {
    collect_numbers(std::initializer_list<unsigned int> numbers) : expected{numbers} {}
    ~collect_numbers()
    {
      std::vector<unsigned int> sorted_actual(std::begin(actual), std::end(actual));
      std::sort(begin(sorted_actual), end(sorted_actual));
      CHECK(expected == sorted_actual);
    }
    void collect(unsigned int const num) { actual.push_back(num); }
    tbb::concurrent_vector<unsigned int> actual;
    std::vector<unsigned int> const expected;
  };

  // Hacky!
  struct check_multiple_numbers {
    check_multiple_numbers(int const n) : total{n} {}
    ~check_multiple_numbers() { CHECK(sum == total); }
    void add_difference(unsigned int const a, unsigned int const b)
    {
      // The difference is calculated to test that add(a, b) yields a different result
      // than add(b, a).
      sum += static_cast<int>(b) - static_cast<int>(a);
    }
    std::atomic<int> sum;
    int const total;
  };

  constexpr bool in_range(unsigned int const b, unsigned int const e, unsigned int const i) noexcept
  {
    return i >= b and i < e;
  }

  struct not_in_range {
    explicit not_in_range(unsigned int const b, unsigned int const e) : begin{b}, end{e} {}
    unsigned int const begin;
    unsigned int const end;
    bool filter(unsigned int const i) const noexcept { return not in_range(begin, end, i); }
  };
}

TEST_CASE("Two filters", "[filtering]")
{
  framework_graph g{[src = source{10u}]() mutable { return src.next(); }};
  g.declare_filter("evens_only", evens_only).concurrency(unlimited).input("num");
  g.declare_filter("odds_only", odds_only).concurrency(unlimited).input("num");
  g.make<sum_numbers>(20u)
    .declare_transform("add_evens", &sum_numbers::add)
    .concurrency(unlimited)
    .filtered_by("evens_only")
    .input("num");
  g.make<sum_numbers>(25u)
    .declare_transform("add_odds", &sum_numbers::add)
    .concurrency(unlimited)
    .filtered_by("odds_only")
    .input("num");

  g.execute("filter_t.gv");
}

TEST_CASE("Two filters in series", "[filtering]")
{
  framework_graph g{[src = source{10u}]() mutable { return src.next(); }};
  g.declare_filter("evens_only", evens_only).concurrency(unlimited).input("num");
  g.declare_filter("odds_only", odds_only)
    .concurrency(unlimited)
    .filtered_by("evens_only")
    .input("num");
  g.make<sum_numbers>(0u)
    .declare_transform("add", &sum_numbers::add)
    .concurrency(unlimited)
    .filtered_by("odds_only")
    .input("num");

  g.execute();
}

TEST_CASE("Two filters in parallel", "[filtering]")
{
  framework_graph g{[src = source{10u}]() mutable { return src.next(); }};
  g.declare_filter("evens_only", evens_only).concurrency(unlimited).input("num");
  g.declare_filter("odds_only", odds_only).concurrency(unlimited).input("num");
  g.make<sum_numbers>(0u)
    .declare_transform("add", &sum_numbers::add)
    .concurrency(unlimited)
    .filtered_by("odds_only", "evens_only")
    .input("num");

  g.execute();
}

TEST_CASE("Three filters in parallel", "[filtering]")
{
  struct filter_config {
    std::string name;
    unsigned int begin;
    unsigned int end;
  };
  std::vector<filter_config> configs{{.name = "exclude_0_to_4", .begin = 0, .end = 4},
                                     {.name = "exclude_6_to_7", .begin = 6, .end = 7},
                                     {.name = "exclude_gt_8", .begin = 8, .end = -1u}};

  framework_graph g{[src = source{10u}]() mutable { return src.next(); }};
  for (auto const& [name, b, e] : configs) {
    g.make<not_in_range>(b, e)
      .declare_filter(name, &not_in_range::filter)
      .concurrency(unlimited)
      .input("num");
  }

  std::vector<std::string> const filter_names{"exclude_0_to_4", "exclude_6_to_7", "exclude_gt_8"};
  auto const expected_numbers = {4u, 5u, 7u};
  g.make<collect_numbers>(expected_numbers)
    .declare_transform("collect", &collect_numbers::collect)
    .concurrency(unlimited)
    .filtered_by(filter_names)
    .input("num");

  g.execute();
}

TEST_CASE("Two filters in parallel (each with multiple arguments)", "[filtering]")
{
  framework_graph g{[src = source{10u}]() mutable { return src.next(); }};
  g.declare_filter("evens_only", evens_only).concurrency(unlimited).input("num");
  g.declare_filter("odds_only", odds_only).concurrency(unlimited).input("num");
  g.make<check_multiple_numbers>(5 * 100)
    .declare_transform("check_evens", &check_multiple_numbers::add_difference)
    .concurrency(unlimited)
    .filtered_by("evens_only")
    .input("num", "other_num"); // <= Note input order
  g.make<check_multiple_numbers>(-5 * 100)
    .declare_transform("check_odds", &check_multiple_numbers::add_difference)
    .concurrency(unlimited)
    .filtered_by("odds_only")
    .input("other_num", "num"); // <= Note input order

  g.execute();
}
