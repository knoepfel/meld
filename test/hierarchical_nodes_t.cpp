#include "meld/core/cached_product_stores.hpp"
#include "meld/core/framework_graph.hpp"
#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"
#include "meld/utilities/debug.hpp"
#include "meld/utilities/sleep_for.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb/flow_graph.h"
#include "oneapi/tbb/global_control.h"

#include <atomic>
#include <cmath>
#include <ctime>
#include <string>
#include <tuple>
#include <vector>

using namespace meld;
using namespace meld::concurrency;
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

  struct threadsafe_data_for_rms {
    std::atomic<unsigned int> total;
    std::atomic<unsigned int> number;
    data_for_rms
    send() const
    {
      return {total.load(), number.load()};
    }
  };

  void
  concat(threadsafe_data_for_rms& redata, unsigned squared_number)
  {
    redata.total += squared_number;
    ++redata.number;
  }

  double
  scale(data_for_rms data)
  {
    return std::sqrt(static_cast<double>(data.total) / data.number);
  }

  std::string
  strtime(std::time_t tm)
  {
    char buffer[32];
    std::strncpy(buffer, std::ctime(&tm), 26);
    return buffer;
  }

  void
  print_result(handle<double> result, std::string const& stringized_time)
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
  auto const b = cbegin(transitions);
  auto const e = cend(transitions);
  cached_product_stores cached_stores;
  std::map<level_id, std::size_t> original_message_ids;
  framework_graph graph{[&cached_stores, &original_message_ids, b, e, it = b](
                          tbb::flow_control& fc) mutable -> message {
    if (it == e) {
      fc.stop();
      return {};
    }
    auto const& [id, stage] = *it++;

    std::size_t const message_id = std::distance(b, it);
    original_message_ids.try_emplace(id, message_id);

    auto store = cached_stores.get_empty_store(id, stage);
    debug("Starting ", id, " with stage ", to_string(stage));

    if (store->is_flush()) {
      std::size_t const original_message_id = original_message_ids.at(store->parent()->id());
      return {store, message_id, original_message_id};
    }
    if (id.depth() == 1ull) {
      store->add_product<std::time_t>("time", std::time(nullptr));
    }
    if (id.depth() == 2ull) {
      store->add_product<unsigned>("number", id.back() + id.parent().back());
    }
    return {store, message_id};
  }};

  auto c = graph.make_component();
  c.declare_transform("get_the_time", strtime)
    .concurrency(unlimited)
    .input("time")
    .output("strtime");
  c.declare_transform("square", square)
    .concurrency(unlimited)
    .input("number")
    .output("squared_number");
  c.declare_reduction("concat", concat, 15u)
    .concurrency(unlimited)
    .input("squared_number")
    .output("concat_data");
  c.declare_transform("scale", scale).concurrency(unlimited).input("concat_data").output("result");
  c.declare_transform("print_result", print_result)
    .concurrency(unlimited)
    .input("result", "strtime");

  graph.execute();
}
