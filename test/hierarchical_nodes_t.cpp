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

  template <std::floating_point T>
  auto
  concat(T left, T right)
  {
    return left + right;
  }

  double
  scale(unsigned int const sum, std::size_t const concat_n)
  {
    return std::sqrt(static_cast<double>(sum) / concat_n);
  }

  template <typename T>
  struct user_data {
    transition tr;
    T data;

    template <typename U>
    user_data<U>
    create(U u) const
    {
      return {tr, std::move(u)};
    }
  };

  template <typename Output, typename = void>
  struct output_type {
    using type = Output;
  };

  template <typename Output>
  struct output_type<Output, std::void_t<typename Output::sent_data>> {
    using type = typename Output::sent_data;
  };

  template <typename Output>
  using sent_data_t = typename output_type<Output>::type;

  template <typename Input, typename Output>
  class reduction_node :
    public flow::multifunction_node<Input, std::tuple<user_data<sent_data_t<Output>>>> {
    using base_t = flow::multifunction_node<Input, std::tuple<user_data<sent_data_t<Output>>>>;

  public:
    template <typename FT>
    reduction_node(flow::graph& g, std::size_t concurrency, FT f) :
      base_t{g, concurrency, [this, user_func = std::move(f)](Input const& data, auto& outputs) {
               auto const& [id, st] = data.tr;
               auto parent_id = id.parent();
               auto it = entries_.find(parent_id);
               if (it == entries_.end()) {
                 it = entries_.insert({parent_id, std::make_unique<map_entry>()}).first;
               }
               auto& [cnt, token_encountered, output] = *it->second;
               if (st == stage::flush) {
                 token_encountered = id.back();
               }
               else {
                 user_func(output, data);
                 ++cnt; // Increment must happen *after* all work is done
               }
               if (cnt == token_encountered) {
                 get<0>(outputs).try_put(
                   {transition{std::move(parent_id), stage::process}, output.send()});
                 // Reclaim some memory; would be better to erase the entire entry from the map,
                 // but that is not thread-safe.
                 it->second.reset();
               }
             }}
    {
    }

  private:
    struct map_entry {
      std::atomic<unsigned int> count{};
      std::atomic<unsigned int> stop_token_encountered_{-1u};
      Output user_output{};
    };
    tbb::concurrent_unordered_map<level_id, std::unique_ptr<map_entry>> entries_;
  };
}

TEST_CASE("Hierarchical nodes", "[graph]")
{
  //  using data_s = user_data<std::string>;
  using data_u = user_data<unsigned>;

  flow::graph g;
  constexpr auto index_limit = 2u;
  constexpr auto number_limit = 5u;
  std::vector<transition> transitions;
  transitions.reserve(index_limit * number_limit + 2);
  for (unsigned i = 0u; i != index_limit; ++i) {
    level_id const id{i};
    for (unsigned j = 0u; j != number_limit; ++j) {
      transitions.emplace_back(id.make_child(j), stage::process);
    }
    transitions.emplace_back(id.make_child(number_limit), stage::flush);
  }
  auto it = cbegin(transitions);
  auto const e = cend(transitions);
  flow::input_node src{g, [it, e](tbb::flow_control& fc) mutable -> data_u {
                         if (it == e) {
                           fc.stop();
                           return {transition{}, 0u};
                         }
                         auto const& [id, st] = *it;
                         unsigned int const data =
                           st == stage::flush ? 0u : id.back() + id.parent().back();
                         return {*it++, data};
                       }};

  // flow::function_node record_id_time{g, flow::unlimited, [](data_u const& d) -> data_u {
  //   if (
  // }};
  flow::function_node square_node{
    g, flow::unlimited, [](data_u const& d) -> data_u { return d.create(square(d.data)); }};

  flow::function_node multiplexer{
    g, flow::unlimited, [&square_node](data_u const& d) -> flow::continue_msg {
      if (d.tr.first.depth() == 2ull) {
        square_node.try_put(d);
      }
      debug("Done ", d.tr.first);
      return {};
    }};

  struct data_for_rms {
    using sent_data = std::pair<unsigned int, unsigned int>;
    std::atomic<unsigned int> total;
    std::atomic<unsigned int> number;

    auto
    send() const
    {
      return sent_data{total.load(), number.load()};
    }
  };

  reduction_node<data_u, data_for_rms> concatenate{
    g, flow::unlimited, [](data_for_rms& redata, data_u squared_num) {
      debug("Adding ", squared_num.data, " (from data packet ", squared_num.tr.first, ")");
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(1s);
      redata.total += squared_num.data;
      ++redata.number;
    }};

  std::map<level_id, double> results;
  flow::function_node rms{
    g,
    flow::serial,
    [&results](user_data<data_for_rms::sent_data> const& redata) -> flow::continue_msg {
      auto const& [tr, user_data] = redata;
      results.emplace(tr.first, scale(user_data.first, user_data.second));
      return {};
    }};

  make_edge(src, multiplexer);

  make_edge(square_node, concatenate);
  make_edge(concatenate, rms);

  src.activate();
  g.wait_for_all();

  for (auto const& [id, rms] : results) {
    debug("  ID: ", id, " RMS: ", rms);
  }
}
