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
#include "test/products_for_output.hpp"

#include "catch2/catch.hpp"
#include "spdlog/spdlog.h"

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
  };

  data_for_rms send(threadsafe_data_for_rms const& data)
  {
    return {meld::send(data.total), meld::send(data.number)};
  }

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
    spdlog::debug("{}: {} @ {}",
                  result.id().to_string(),
                  *result,
                  stringized_time.substr(0, stringized_time.find('\n')));
  }
}

TEST_CASE("Hierarchical nodes", "[graph]")
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

    if (id->level_name() == "run") {
      store->add_product<std::time_t>("time", std::time(nullptr));
    }
    if (id->level_name() == "event") {
      store->add_product<unsigned>("number", id->number() + id->parent()->number());
    }
    return store;
  }, 1};

  g.with("get_the_time", strtime)
    .filtered_by()
    .using_concurrency(unlimited)
    .transform("time")
    .to("strtime");
  g.with(square).using_concurrency(unlimited).transform("number").to("squared_number");
  g.declare_reduction(add, 15u)
    .filtered_by()
    .concurrency(unlimited)
    .react_to("squared_number")
    .output("added_data")
    .over("run");

  g.with(scale).using_concurrency(unlimited).transform("added_data").to("result");
  g.with(print_result).using_concurrency(unlimited).monitor("result", "strtime");

  g.make<test::products_for_output>()
    .declare_output(&test::products_for_output::save)
    .filtered_by();

  g.execute("hierarchical_nodes_t.gv");

  CHECK(g.execution_counts("square") == index_limit * number_limit);
  CHECK(g.execution_counts("add") == index_limit * number_limit);
  CHECK(g.execution_counts("get_the_time") >= index_limit);
  CHECK(g.execution_counts("scale") == index_limit);
  CHECK(g.execution_counts("print_result") == index_limit);
}
