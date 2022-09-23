#include "meld/graph/make_edges.hpp"
#include "meld/graph/serial_node.hpp"
#include "meld/utilities/debug.hpp"
#include "meld/utilities/thread_counter.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb/flow_graph.h"

#include <atomic>
#include <string>

using namespace meld;
using namespace oneapi::tbb;

TEST_CASE("Serialize functions based on resource", "[multithreading]")
{
  flow::graph g;
  unsigned int i{};
  flow::input_node src{g, [&i](flow_control& fc) {
                         if (i < 10) {
                           return ++i;
                         }
                         fc.stop();
                         return 0u;
                       }};

  serializers serialized_resources{g};

  std::atomic<unsigned int> root_counter{}, genie_counter{};

  serial_node<unsigned int, 1> node1{
    g, serialized_resources.get("ROOT"), [&root_counter](unsigned int const i) {
      thread_counter c{root_counter};
      debug("Processing from node 1 ", i);
      return i;
    }};

  serial_node<unsigned int, 2> node2{g,
                                     serialized_resources.get("ROOT", "GENIE"),
                                     [&root_counter, &genie_counter](unsigned int const i) {
                                       thread_counter c1{root_counter};
                                       thread_counter c2{genie_counter};
                                       debug("Processing from node 2 ", i);
                                       return i;
                                     }};

  serial_node<unsigned int, 1> node3{
    g, serialized_resources.get("GENIE"), [&genie_counter](unsigned int const i) {
      thread_counter c{genie_counter};
      debug("Processing from node 3 ", i);
      return i;
    }};

  serial_node<unsigned int, 0> node4{
    g, tbb::flow::unlimited, [](unsigned int const i) { return i; }};

  auto receiving_node_for = [](tbb::flow::graph& g, std::string const& label) {
    return flow::function_node<unsigned int, unsigned int>{
      g, flow::unlimited, [&label](unsigned int const i) {
        debug("Processed ", label, " task ", i);
        return i;
      }};
  };

  auto receiving_node_1 = receiving_node_for(g, "ROOT");
  auto receiving_node_2 = receiving_node_for(g, "ROOT/GENIE");
  auto receiving_node_3 = receiving_node_for(g, "GENIE");
  auto receiving_node_4 = receiving_node_for(g, "unlimited");

  nodes(src)->nodes(node1, node2, node3, node4);
  nodes(node1)->nodes(receiving_node_1);
  nodes(node2)->nodes(receiving_node_2);
  nodes(node3)->nodes(receiving_node_3);
  nodes(node4)->nodes(receiving_node_4);

  serialized_resources.activate();
  src.activate();
  g.wait_for_all();
}

TEST_CASE("Serialize functions in split/merge graph", "[multithreading]")
{
  flow::graph g;
  flow::input_node src{g, [i = 0u](flow_control& fc) mutable {
                         if (i < 10u) {
                           return ++i;
                         }
                         fc.stop();
                         return 0u;
                       }};

  serializers serialized_resources{g};

  std::atomic<unsigned int> root_counter{};

  auto root_resource = serialized_resources.get("ROOT");
  auto serial_node_for = [&root_resource, &root_counter](auto& g, int label) {
    return serial_node<unsigned int, 1>{
      g, root_resource, [&root_counter, label](unsigned int const i) {
        thread_counter c{root_counter};
        debug("Processing from node ", label, ' ', i);
        return i;
      }};
  };

  auto node1 = serial_node_for(g, 1);
  auto node2 = serial_node_for(g, 2);
  auto node3 = serial_node_for(g, 3);

  nodes(src)->nodes(node1, node2)->nodes(node3);

  serialized_resources.activate();
  src.activate();
  g.wait_for_all();
}
