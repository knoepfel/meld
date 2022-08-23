#include "meld/core/cached_product_stores.hpp"
#include "meld/core/framework_graph.hpp"
#include "meld/core/make_component.hpp"
#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"
#include "meld/utilities/debug.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb/concurrent_unordered_map.h"
#include "oneapi/tbb/flow_graph.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <concepts>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

using namespace meld;
using namespace oneapi::tbb;

namespace {
  auto
  square(unsigned int const num)
  {
    return num * num;
  }

  struct data_for_rms {
    unsigned int total;
    unsigned int number;
  };

  void
  concat(data_for_rms& redata, unsigned squared_number)
  {
    redata.total += squared_number;
    ++redata.number;
  }

  double
  scale(data_for_rms data)
  {
    return std::sqrt(static_cast<double>(data.total) / data.number);
  }
}

TEST_CASE("Hierarchical nodes", "[graph]")
{
  auto c = make_component();
  c.declare_transform("square", square)
    .concurrency(flow::unlimited)
    .input("number")
    .output("squared_number");
  c.declare_reduction("concat", concat, data_for_rms{.total = 15})
    .concurrency(flow::serial)
    .input("squared_number")
    .output("concat_data");
  c.declare_transform("scale", scale)
    .concurrency(flow::serial)
    .input("concat_data")
    .output("result");

  constexpr auto index_limit = 2u;
  constexpr auto number_limit = 5u;
  std::vector<transition> transitions;
  transitions.reserve(index_limit * (number_limit + 1u));
  for (unsigned i = 0u; i != index_limit; ++i) {
    level_id const id{i};
    for (unsigned j = 0u; j != number_limit; ++j) {
      transitions.emplace_back(id.make_child(j), stage::process);
    }
    transitions.emplace_back(id.make_child(number_limit), stage::flush);
  }
  auto it = cbegin(transitions);
  auto const e = cend(transitions);
  cached_product_stores cached_stores;
  framework_graph graph{
    [&cached_stores, it, e](tbb::flow_control& fc) mutable -> product_store_ptr {
      if (it == e) {
        fc.stop();
        return {};
      }
      auto const& [id, st] = *it++;
      bool const is_flush = st == stage::flush;

      auto store = cached_stores.get_store(id, is_flush);
      debug("Starting ", id, " with stage ", to_string(st));
      if (not is_flush) {
        store->add_product<unsigned>("number", id.back() + id.parent().back());
      }
      return store;
    }};

  graph.merge(c.release_transforms(), c.release_reductions());
  graph.finalize_and_run();

  // // flow::function_node record_id_time{g, flow::unlimited, [](data_u const& d) -> data_u {
  // //   if (
  // // }};

  for (auto const& id : {"0"_id, "1"_id}) {
    auto const& store = cached_stores.get_store(id);
    debug("  ID: ", id, " RMS: ", store->get_product<double>("result"));
  }
}
