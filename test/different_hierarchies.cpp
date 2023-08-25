// =======================================================================================
// This test executes the following graph
//
//        Multiplexer
//        |         |
//    job_add(*) run_add(^)
//        |         |
//        |     verify_run_sum
//        |
//   verify_job_sum
//
// where the asterisk (*) indicates a reduction step over the full job, and the caret (^)
// represents a reduction step over each run.
//
// The hierarchy tested is:
//
//    job
//     │
//     ├ trigger primitive
//     │
//     └ run
//        │
//        └ event
//
// As the run_add node performs reductions only over "runs", any "trigger primitive"
// stores are excluded from the reduction result.
// =======================================================================================

#include "meld/core/cached_product_stores.hpp"
#include "meld/core/framework_graph.hpp"
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
  void add(std::atomic<unsigned int>& counter, unsigned int number) { counter += number; }
}

TEST_CASE("Different hierarchies used with reduction", "[graph]")
{
  // job -> run -> event levels
  constexpr auto index_limit = 2u;
  constexpr auto number_limit = 5u;
  std::vector<level_id_ptr> levels;
  auto job_id = levels.emplace_back(level_id::base_ptr());
  for (unsigned i = 0u; i != index_limit; ++i) {
    auto run_id = levels.emplace_back(job_id->make_child(i, "run"));
    for (unsigned j = 0u; j != number_limit; ++j) {
      levels.push_back(run_id->make_child(j, "event"));
    }
  }

  // job -> trigger primitive levels
  constexpr auto primitive_limit = 10u;
  for (unsigned i = 0u; i != primitive_limit; ++i) {
    levels.push_back(job_id->make_child(i, "trigger primitive"));
  }

  auto it = cbegin(levels);
  auto const e = cend(levels);
  framework_graph g{[it, e](cached_product_stores& cached_stores) mutable -> product_store_ptr {
    if (it == e) {
      return nullptr;
    }
    auto const& id = *it++;

    auto store = cached_stores.get_store(id);
    // Insert a "number" for either events or trigger primitives
    if (id->level_name() == "event" or id->level_name() == "trigger primitive") {
      store->add_product<unsigned>("number", id->number());
    }
    return store;
  }};

  g.declare_reduction("run_add", add, 0u)
    .concurrency(unlimited)
    .react_to("number")
    .output("run_sum")
    .over("run");
  g.declare_reduction("job_add", add, 0u)
    .concurrency(unlimited)
    .react_to("number")
    .output("job_sum")
    .over("job");

  g.with("verify_run_sum", [](unsigned int actual) { CHECK(actual == 10u); })
    .monitor("run_sum")
    .using_concurrency(unlimited);
  g.with("verify_job_sum",
         [](unsigned int actual) {
           CHECK(actual == 20u + 45u); // 20u from events, 45u from trigger primitives
         })
    .monitor("job_sum")
    .using_concurrency(unlimited);

  g.execute();

  CHECK(g.execution_counts("run_add") == index_limit * number_limit);
  CHECK(g.execution_counts("job_add") == index_limit * number_limit + primitive_limit);
  CHECK(g.execution_counts("verify_run_sum") == index_limit);
  CHECK(g.execution_counts("verify_job_sum") == 1);
}
