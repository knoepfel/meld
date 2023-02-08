// =======================================================================================
// This test executes the following graph
//
//     Multiplexer
//      |       |
//   get_time square
//      |       |
//      |      add(*)
//      |       |
//      |     scale
//      |       |
//     print_result [also includes output module]
//
// where the asterisk (*) indicates a reduction step.  In terms of the data model,
// whenever the add node receives the flush token, a product is inserted at one level
// higher than the level processed by square and add nodes.
// =======================================================================================

#include "meld/core/cached_product_stores.hpp"
#include "meld/core/framework_graph.hpp"
#include "meld/model/level_hierarchy.hpp"
#include "meld/model/level_id.hpp"
#include "meld/model/product_store.hpp"

#include "catch2/catch.hpp"
#include "spdlog/spdlog.h"

#include <atomic>
#include <string>
#include <vector>

using namespace meld;
using namespace meld::concurrency;

namespace {
  struct counter {
    std::atomic<unsigned int> number;
    auto send() const { return number.load(); }
  };

  void add(counter& result, unsigned int number) { result.number += number; }
  void verify(unsigned int sum, unsigned expected) { CHECK(sum == expected); }
}

TEST_CASE("Different levels of reduction", "[graph]")
{
  constexpr auto index_limit = 2u;
  constexpr auto number_limit = 5u;
  std::vector<level_id_ptr> levels;
  levels.reserve(1 + index_limit * (number_limit + 1u));
  levels.push_back(level_id::base_ptr());
  for (unsigned i = 0u; i != index_limit; ++i) {
    auto id = level_id::base().make_child(i, "run");
    levels.push_back(id);
    for (unsigned j = 0u; j != number_limit; ++j) {
      levels.push_back(id->make_child(j, "event"));
    }
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
    if (id->level_name() == "event") {
      store->add_product<unsigned>("number", id->number());
    }
    return store;
  }};

  g.declare_reduction("run_add", add, 0)
    .concurrency(unlimited)
    .react_to("number")
    .output("run_sum")
    .over("run");
  g.declare_reduction("job_add", add, 0)
    .concurrency(unlimited)
    .react_to("run_sum")
    .output("job_sum")
    .over("job");
  g.declare_reduction("other_job_add", add, 0)
    .concurrency(unlimited)
    .react_to("number")
    .output("other_job_sum")
    .over("job");

  g.declare_monitor("verify_run_sum", verify)
    .concurrency(unlimited)
    .input(react_to("run_sum"), use(10u));
  g.declare_monitor("verify_job_sum", verify)
    .concurrency(unlimited)
    .input(react_to("job_sum"), use(20u));
  g.declare_monitor("verify_other_job_sum", verify)
    .concurrency(unlimited)
    .input(react_to("other_job_sum"), use(20u));

  g.execute();

  CHECK(g.execution_counts("run_add") == index_limit * number_limit);
  CHECK(g.execution_counts("job_add") == index_limit);
  CHECK(g.execution_counts("other_job_add") == index_limit * number_limit);
  CHECK(g.execution_counts("verify_run_sum") == index_limit);
  CHECK(g.execution_counts("verify_job_sum") == 1);
  CHECK(g.execution_counts("verify_other_job_sum") == 1);
}
