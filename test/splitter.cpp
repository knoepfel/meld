// =======================================================================================
// This test executes splitting functionality using the following graph
//
//     Multiplexer
//          |
//      splitter (creates children)
//          |
//         add(*)
//          |
//     print_result
//
// where the asterisk (*) indicates a reduction step.  The difference here is that the
// *splitter* is responsible for sending the flush token instead of the
// source/multiplexer.
// =======================================================================================

#include "meld/core/cached_product_stores.hpp"
#include "meld/core/framework_graph.hpp"
#include "meld/model/level_id.hpp"
#include "meld/model/product_store.hpp"
#include "test/products_for_output.hpp"

#include "catch2/catch.hpp"

#include <atomic>
#include <string>
#include <vector>

using namespace meld;
using namespace meld::concurrency;

namespace {
  class iota {
  public:
    explicit iota(unsigned int max_number) : max_{max_number} {}
    unsigned int initial_value() const { return 0; }
    bool predicate(unsigned int i) const { return i != max_; }
    auto unfold(unsigned int i) const { return std::make_pair(i + 1, i); };

  private:
    unsigned int max_;
  };

  using numbers_t = std::vector<int>;

  class iterate_through {
  public:
    explicit iterate_through(numbers_t const& numbers) :
      begin_{numbers.begin()}, end_{numbers.end()}
    {
    }
    auto initial_value() const { return begin_; }
    bool predicate(numbers_t::const_iterator it) const { return it != end_; }
    auto unfold(numbers_t::const_iterator it) const
    {
      auto num = *it;
      return std::make_pair(++it, num);
    };

  private:
    numbers_t::const_iterator begin_;
    numbers_t::const_iterator end_;
  };

  void add(std::atomic<unsigned int>& counter, unsigned number) { counter += number; }

  void check_sum(handle<unsigned int> const sum)
  {
    if (sum.id().number() == 0ull) {
      CHECK(*sum == 45);
    }
    else {
      CHECK(*sum == 190);
    }
  }
}

TEST_CASE("Splitting the processing", "[graph]")
{
  constexpr auto index_limit = 2u;
  std::vector<level_id_ptr> levels;
  levels.reserve(index_limit + 1u);
  levels.push_back(level_id::base_ptr());
  for (unsigned i = 0u; i != index_limit; ++i) {
    levels.push_back(level_id::base().make_child(i, "event"));
  }

  auto it = cbegin(levels);
  auto const e = cend(levels);
  cached_product_stores cached_stores{};
  framework_graph g{[&cached_stores, it, e]() mutable -> product_store_ptr {
    if (it == e) {
      return nullptr;
    }
    auto const& id = *it++;

    auto store = cached_stores.get_store(id);
    if (store->id()->level_name() == "event") {
      store->add_product<unsigned>("max_number", 10u * (id->number() + 1));
      store->add_product<numbers_t>("same_numbers", numbers_t(10, id->number() + 1));
    }
    return store;
  }};

  g.with<iota>(&iota::predicate, &iota::unfold)
    .using_concurrency(unlimited)
    .split("max_number")
    .into("num")
    .within_domain("lower");
  g.with<iterate_through>(&iterate_through::predicate, &iterate_through::unfold)
    .using_concurrency(unlimited)
    .split("same_numbers")
    .into("same_num")
    .within_domain("lower");
  g.declare_reduction(add).concurrency(unlimited).react_to("num").output("sum").over("event");
  g.with(check_sum).using_concurrency(unlimited).monitor("sum");
  g.make<test::products_for_output>()
    .declare_output(&test::products_for_output::save)
    .concurrency(serial);

  g.execute("splitter_t.gv");

  CHECK(g.execution_counts("iota") == index_limit);
  CHECK(g.execution_counts("iterate_through") == index_limit);
  CHECK(g.execution_counts("add") == 30);
  CHECK(g.execution_counts("check_sum") == index_limit);
}
