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

  using numbers_t = std::vector<unsigned int>;

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
  void add_numbers(std::atomic<unsigned int>& counter, unsigned number) { counter += number; }

  void check_sum(handle<unsigned int> const sum)
  {
    if (sum.level_id().number() == 0ull) {
      CHECK(*sum == 45);
    }
    else {
      CHECK(*sum == 190);
    }
  }

  void check_sum_same(handle<unsigned int> const sum)
  {
    auto const expected_sum = (sum.level_id().number() + 1) * 10;
    CHECK(*sum == expected_sum);
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
  framework_graph g{[it, e](cached_product_stores& cached_stores) mutable -> product_store_ptr {
    if (it == e) {
      return nullptr;
    }
    auto const& id = *it++;

    auto store = cached_stores.get_store(id);
    if (store->id()->level_name() == "event") {
      store->add_product<unsigned>("max_number", 10u * (id->number() + 1));
      store->add_product<numbers_t>("ten_numbers", numbers_t(10, id->number() + 1));
    }
    return store;
  }};

  g.with<iota>(&iota::predicate, &iota::unfold, concurrency::unlimited)
    .split("max_number")
    .into("new_number")
    .within_domain("lower1");
  g.with(add, concurrency::unlimited).reduce("new_number").for_each("event").to("sum1");
  g.with(check_sum, concurrency::unlimited).monitor("sum1");

  g.with<iterate_through>(
     &iterate_through::predicate, &iterate_through::unfold, concurrency::unlimited)
    .split("ten_numbers")
    .into("each_number")
    .within_domain("lower2");
  g.with(add_numbers, concurrency::unlimited).reduce("each_number").for_each("event").to("sum2");
  g.with(check_sum_same, concurrency::unlimited).monitor("sum2");

  g.make<test::products_for_output>().output_with(&test::products_for_output::save,
                                                  concurrency::serial);

  g.execute("splitter_t.gv");

  CHECK(g.execution_counts("iota") == index_limit);
  CHECK(g.execution_counts("add") == 30);
  CHECK(g.execution_counts("check_sum") == index_limit);

  CHECK(g.execution_counts("iterate_through") == index_limit);
  CHECK(g.execution_counts("add_numbers") == 20);
  CHECK(g.execution_counts("check_sum_same") == index_limit);
}
