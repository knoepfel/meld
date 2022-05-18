#include "meld/utilities/debug.hpp"
#include "meld/utilities/thread_counter.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb/flow_graph.h"

using namespace meld;
using namespace oneapi::tbb;
using namespace std::chrono_literals;

namespace {
  struct thread_unsafe {
    unsigned int messages_processed{};
    std::atomic<unsigned int> counter{};

    auto
    increment(unsigned int i)
    {
      thread_counter c{counter};
      ++messages_processed;
      return i;
    }
  };

  // template <typename Input>
  // using replicated_node_base = tbb::flow::component_node<std::tuple<Input>, std::tuple<Input>;

  // template <typename Input, std::size_t N>
  // class replicated_node : public replicated_node_base<Input> {
  // public:
  //   template <typename FT>
  //   replicated_node(tbb::flow::graph& g, FT ft)
  //     : buffer_{g}
  //     ,
  // public:
  //   tbb::flow::buffer_node buffer_;
  //   std::vector<tbb::flow::function_node<Input, Input, tbb::flow::rejecting>> nodes_;
  // };
}

TEST_CASE("Replicated function calls", "[multithreading]")
{
  constexpr unsigned int total_messages{10u};
  flow::graph g;
  flow::input_node src{g, [i = 0u](flow_control& fc) mutable {
                         if (i < total_messages) {
                           return ++i;
                         }
                         fc.stop();
                         return 0u;
                       }};

  flow::buffer_node<unsigned int> buffer{g};
  make_edge(src, buffer);

  std::vector<thread_unsafe> modules(4);
  std::vector<flow::function_node<unsigned int, unsigned int, flow::rejecting>> nodes;
  nodes.reserve(4);
  for (auto& module : modules) {
    auto& node = nodes.emplace_back(
      g, flow::serial, [&module](unsigned int i) { return module.increment(i); });
    make_edge(buffer, node);
  }

  src.activate();
  g.wait_for_all();

  unsigned int processed_messages{};
  for (auto const& module : modules) {
    processed_messages += module.messages_processed;
  }
  CHECK(processed_messages == total_messages);
}
