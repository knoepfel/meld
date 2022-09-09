#include "meld/core/cached_product_stores.hpp"
#include "meld/core/framework_graph.hpp"
#include "meld/graph/transition.hpp"
#include "meld/utilities/debug.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb/flow_graph.h"

using namespace meld;
using namespace meld::concurrency;

namespace {
  struct OneArg {
    explicit OneArg(std::atomic<unsigned int>& counter) : calls{counter} {}
    int
    call(int) noexcept
    {
      ++calls;
      return 1;
    }
    std::atomic<unsigned int>& calls;
  };

  struct TwoArgs {
    explicit TwoArgs(std::atomic<unsigned int>& counter) : calls{counter} {}
    int
    call(int, int) noexcept
    {
      ++calls;
      return 2;
    }
    std::atomic<unsigned int>& calls;
  };
}

TEST_CASE("Cached function calls", "[data model]")
{
  constexpr std::size_t n_runs{1};
  constexpr std::size_t n_subruns{2u};
  constexpr std::size_t n_events{5u};

  std::vector<transition> transitions;
  for (std::size_t i = 0; i != n_runs; ++i) {
    level_id const run_id{i};
    transitions.emplace_back(run_id, stage::process);
    for (std::size_t j = 0; j != n_subruns; ++j) {
      auto const subrun_id = run_id.make_child(j);
      transitions.emplace_back(subrun_id, stage::process);
      for (std::size_t k = 0; k != n_events; ++k) {
        transitions.emplace_back(subrun_id.make_child(k), stage::process);
      }
      transitions.emplace_back(subrun_id.make_child(n_events), stage::flush);
    }
    transitions.emplace_back(run_id.make_child(n_subruns), stage::flush);
  }
  transitions.emplace_back(level_id{n_runs}, stage::flush);

  auto it = cbegin(transitions);
  auto const end = cend(transitions);
  cached_product_stores stores;
  framework_graph g{[&stores, it, end](tbb::flow_control& fc) mutable -> product_store_ptr {
    if (it == end) {
      fc.stop();
      return nullptr;
    }
    auto const& [id, stage] = *it++;

    auto store = stores.get_store(id, stage);
    if (store->is_flush()) {
      return store;
    }
    if (store->id().depth() == 1ull) {
      store->add_product<int>("number", 2 * store->id().back());
    }
    if (store->id().depth() == 2ull) {
      store->add_product<int>("another", 3 * store->id().back());
    }
    if (store->id().depth() == 3ull) {
      store->add_product<int>("still", 4 * store->id().back());
    }
    return store;
  }};

  std::atomic<unsigned int> a1_counter{};
  g.make_component<OneArg>(a1_counter)
    .declare_transform("A1", &OneArg::call)
    .concurrency(unlimited)
    .input("number")
    .output("one");
  std::atomic<unsigned int> a2_counter{};
  g.make_component<OneArg>(a2_counter)
    .declare_transform("A2", &OneArg::call)
    .concurrency(unlimited)
    .input("one")
    .output("used_one");
  std::atomic<unsigned int> a3_counter{};
  g.make_component<OneArg>(a3_counter)
    .declare_transform("A3", &OneArg::call)
    .concurrency(unlimited)
    .input("used_one")
    .output("done_one");

  std::atomic<unsigned int> b1_counter{};
  g.make_component<TwoArgs>(b1_counter)
    .declare_transform("B1", &TwoArgs::call)
    .concurrency(unlimited)
    .input("one", "another")
    .output("two");
  std::atomic<unsigned int> b2_counter{};
  g.make_component<TwoArgs>(b2_counter)
    .declare_transform("B2", &TwoArgs::call)
    .concurrency(unlimited)
    .input("used_one", "two")
    .output("used_two");

  std::atomic<unsigned int> c_counter{};
  g.make_component<TwoArgs>(c_counter)
    .declare_transform("C", &TwoArgs::call)
    .concurrency(unlimited)
    .input("used_two", "still")
    .output("three");

  g.execute();

  CHECK(a1_counter == 1u);
  CHECK(a2_counter == 1u);
  CHECK(a3_counter == 1u);

  CHECK(b1_counter == 2u);
  CHECK(b2_counter == 2u);

  CHECK(c_counter == 10u);
}
