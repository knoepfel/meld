#include "meld/graph/serial_node.hpp"
#include "meld/utilities/debug.hpp"
#include "meld/utilities/thread_counter.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb/flow_graph.h"

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

  flow::function_node<unsigned int, unsigned int> receiving_node_1{
    g, flow::unlimited, [](unsigned int const i) {
      debug("Processed ROOT task ", i);
      return i;
    }};

  flow::function_node<unsigned int, unsigned int> receiving_node_2{
    g, flow::unlimited, [](unsigned int const i) {
      debug("Processed ROOT/GENIE task ", i);
      return i;
    }};

  flow::function_node<unsigned int, unsigned int> receiving_node_3{
    g, flow::unlimited, [](unsigned int const i) {
      debug("Processed GENIE task ", i);
      return i;
    }};

  flow::function_node<unsigned int, unsigned int> receiving_node_4{
    g, flow::unlimited, [](unsigned int const i) {
      debug("Processed unlimited task ", i);
      return i;
    }};

  make_edge(src, node1);
  make_edge(src, node2);
  make_edge(src, node3);
  make_edge(src, node4);
  make_edge(node1, receiving_node_1);
  make_edge(node2, receiving_node_2);
  make_edge(node3, receiving_node_3);
  make_edge(node4, receiving_node_4);

  serialized_resources.activate();
  src.activate();
  g.wait_for_all();
}

TEST_CASE("Serialize functions in sequence", "[multithreading]")
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

  std::atomic<unsigned int> root_counter{};

  serial_node<unsigned int, 1> node1{
    g, serialized_resources.get("ROOT"), [&root_counter](unsigned int const i) {
      thread_counter c{root_counter};
      debug("Processing from node 1 ", i);
      return i;
    }};

  serial_node<unsigned int, 1> node2{
    g, serialized_resources.get("ROOT"), [&root_counter](unsigned int const i) {
      thread_counter c{root_counter};
      debug("Processing from node 2 ", i);
      return i;
    }};

 serial_node<unsigned int, 1> node3{
    g, serialized_resources.get("ROOT"), [&root_counter](unsigned int const i) {
      thread_counter c{root_counter};
      debug("Processing from node 3 ", i);
      return i;
    }};

  make_edge(src, node1);
  make_edge(src, node2);
  make_edge(node1, node3);
  make_edge(node2, node3);

  serialized_resources.activate();
  src.activate();
  g.wait_for_all();
}
