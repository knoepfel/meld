// =======================================================================================
/*
   This test executes the following graph

              Multiplexer
              /        \
             /          \
        job_add(*)     run_add(^)
            |             |\
            |             | \
            |             |  \
     verify_job_sum       |   \
                          |    \
               verify_run_sum   \
                                 \
                             two_layer_job_add(*)
                                  |
                                  |
                           verify_two_layer_job_sum

   where the asterisk (*) indicates a reduction step over the full job, and the caret (^)
   represents a reduction step over each run.
*/
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

namespace {
  void add(std::atomic<unsigned int>& counter, unsigned int number) { counter += number; }
}

TEST_CASE("Different levels of reduction", "[graph]")
{
  constexpr auto index_limit = 2u;
  constexpr auto number_limit = 5u;
  std::vector<level_id_ptr> levels;
  levels.reserve(1 + index_limit * (number_limit + 1u));
  auto job_id = levels.emplace_back(level_id::base_ptr());
  for (unsigned i = 0u; i != index_limit; ++i) {
    auto run_id = levels.emplace_back(job_id->make_child(i, "run"));
    for (unsigned j = 0u; j != number_limit; ++j) {
      levels.push_back(run_id->make_child(j, "event"));
    }
  }

  auto it = cbegin(levels);
  auto const e = cend(levels);
  framework_graph g{[it, e](cached_product_stores& cached_stores) mutable -> product_store_ptr {
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

  g.with("run_add", add, concurrency::unlimited).reduce("number").for_each("run").to("run_sum");
  g.with("job_add", add, concurrency::unlimited).reduce("number").to("job_sum");
  g.with("two_layer_job_add", add, concurrency::unlimited)
    .reduce("run_sum")
    .to("two_layer_job_sum");

  g.with(
     "verify_run_sum", [](unsigned int actual) { CHECK(actual == 10u); }, concurrency::unlimited)
    .monitor("run_sum");
  g.with(
     "verify_two_layer_job_sum",
     [](unsigned int actual) { CHECK(actual == 20u); },
     concurrency::unlimited)
    .monitor("two_layer_job_sum");
  g.with(
     "verify_job_sum", [](unsigned int actual) { CHECK(actual == 20u); }, concurrency::unlimited)
    .monitor("job_sum");

  g.execute();

  CHECK(g.execution_counts("run_add") == index_limit * number_limit);
  CHECK(g.execution_counts("job_add") == index_limit * number_limit);
  CHECK(g.execution_counts("two_layer_job_add") == index_limit);
  CHECK(g.execution_counts("verify_run_sum") == index_limit);
  CHECK(g.execution_counts("verify_two_layer_job_sum") == 1);
  CHECK(g.execution_counts("verify_job_sum") == 1);
}
