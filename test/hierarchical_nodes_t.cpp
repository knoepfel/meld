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
#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"
#include "meld/utilities/debug.hpp"
#include "test/products_for_output.hpp"

#include "catch2/catch.hpp"

#include <atomic>
#include <cmath>
#include <ctime>
#include <string>
#include <vector>

using namespace meld;
using namespace meld::concurrency;

namespace {
  auto square(unsigned int const num) { return num * num; }

  struct data_for_rms {
    unsigned int total;
    unsigned int number;
  };

  struct threadsafe_data_for_rms {
    std::atomic<unsigned int> total;
    std::atomic<unsigned int> number;
    data_for_rms send() const { return {total.load(), number.load()}; }
  };

  void add(threadsafe_data_for_rms& redata, unsigned squared_number)
  {
    redata.total += squared_number;
    ++redata.number;
  }

  double scale(data_for_rms data)
  {
    return std::sqrt(static_cast<double>(data.total) / data.number);
  }

  std::string strtime(std::time_t tm)
  {
    char buffer[32];
    std::strncpy(buffer, std::ctime(&tm), 26);
    return buffer;
  }

  void print_result(handle<double> result, std::string const& stringized_time)
  {
    debug(result.id(), ": ", *result, " @ ", stringized_time);
  }
}

TEST_CASE("Hierarchical nodes", "[graph]")
{
  constexpr auto index_limit = 2u;
  constexpr auto number_limit = 5u;
  std::vector<transition> transitions;
  transitions.reserve(index_limit * (number_limit + 1u));
  for (unsigned i = 0u; i != index_limit; ++i) {
    level_id const id{i};
    transitions.emplace_back(id, stage::process);
    for (unsigned j = 0u; j != number_limit; ++j) {
      transitions.emplace_back(id.make_child(j), stage::process);
    }
    transitions.emplace_back(id.make_child(number_limit), stage::flush);
  }
  auto it = cbegin(transitions);
  auto const e = cend(transitions);
  cached_product_stores cached_stores;
  framework_graph g{[&cached_stores, it, e]() mutable -> product_store_ptr {
    if (it == e) {
      return nullptr;
    }
    auto const& [id, stage] = *it++;

    auto store = cached_stores.get_empty_store(id, stage);
    debug("Starting ", id, " with stage ", to_string(stage));

    if (store->is_flush()) {
      return store;
    }
    if (id.depth() == 1ull) {
      store->add_product<std::time_t>("time", std::time(nullptr));
    }
    if (id.depth() == 2ull) {
      store->add_product<unsigned>("number", id.back() + id.parent().back());
    }
    return store;
  }};

  g.declare_transform("get_the_time", strtime)
    .filtered_by()
    .concurrency(unlimited)
    .input("time")
    .output("strtime");
  g.declare_transform("square", square)
    .concurrency(unlimited)
    .input("number")
    .output("squared_number");
  g.declare_reduction("add", add, 15u)
    .filtered_by()
    .concurrency(unlimited)
    .input("squared_number")
    .output("added_data");
  g.declare_transform("scale", scale).concurrency(unlimited).input("added_data").output("result");
  g.declare_monitor("print_result", print_result).concurrency(unlimited).input("result", "strtime");

  auto c = g.make<test::products_for_output>();
  c.declare_output("save_to_file", &test::products_for_output::save).filtered_by().concurrency(1);

  g.execute("hierarchical_nodes_t.gv");
}
