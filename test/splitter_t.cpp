// ===================================================================
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
//     print_result
//
// where the asterisk (*) indicates a reduction step.  In terms of the
// data model, whenever the add node receives the flush token, a
// product is inserted at one level higher than the level processed by
// square and add nodes.
// ===================================================================

#include "meld/core/cached_product_stores.hpp"
#include "meld/core/framework_graph.hpp"
#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"
#include "meld/utilities/debug.hpp"

#include "catch2/catch.hpp"

#include <atomic>
#include <cmath>
#include <ctime>
#include <string>
#include <vector>

using namespace meld;
using namespace meld::concurrency;

namespace {
  struct data_for_rms {
    unsigned int total;
    unsigned int number;
  };

  struct threadsafe_data {
    std::atomic<unsigned int> total;
    std::atomic<unsigned int> number;
    unsigned int
    send() const
    {
      return total.load();
    }
  };

  void
  add(threadsafe_data& redata, unsigned number)
  {
    redata.total += number;
  }

  void
  split(generator& g, unsigned max_number)
  {
    for (std::size_t i = 0; i != max_number; ++i) {
      g.make_child(i);
    }
  }

  void
  print_sum(handle<unsigned int> const sum)
  {
    debug(sum.id(), ": ", *sum);
  }
}

TEST_CASE("Splitting the processing", "[graph]")
{
  constexpr auto index_limit = 2u;
  std::vector<transition> transitions;
  transitions.reserve(index_limit + 2u);
  level_id const root_id{};
  transitions.emplace_back(root_id, stage::process);
  for (unsigned i = 0u; i != index_limit; ++i) {
    auto const id = root_id.make_child(i);
    transitions.emplace_back(id, stage::process);
    transitions.emplace_back(id.make_child(1u), stage::flush);
  }
  transitions.emplace_back(root_id.make_child(2u), stage::flush);
  auto it = cbegin(transitions);
  auto const e = cend(transitions);
  cached_product_stores cached_stores;
  framework_graph graph{[&cached_stores, it, e]() mutable -> product_store_ptr {
    if (it == e) {
      return nullptr;
    }
    auto const& [id, stage] = *it++;

    auto store = cached_stores.get_empty_store(id, stage);
    debug("Starting ", id, " with stage ", to_string(stage));

    if (store->is_flush()) {
      return store;
    }
    if (store->id().depth() == 0ull) {
      return store;
    }
    store->add_product<unsigned>("max_number", 10u * (id.back() + 1));
    return store;
  }};

  auto c = graph.make_component();
  c.declare_splitter("split", split).concurrency(unlimited).input("max_number"); //.provides("num");
  c.declare_reduction("add", add).concurrency(unlimited).input("num").output("sum");
  c.declare_transform("print_sum", print_sum).concurrency(unlimited).input("sum");

  graph.execute();
}
