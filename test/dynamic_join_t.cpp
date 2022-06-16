#include "meld/graph/dynamic_join_node.hpp"
#include "meld/graph/serial_node.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb/flow_graph.h"

using namespace meld;
using namespace oneapi::tbb;

namespace {
  auto
  input_source(flow::graph& g)
  {
    return flow::input_node<int>{g, [i = 0](flow_control& fc) mutable {
                                   if (i == 10) {
                                     fc.stop();
                                     return -1;
                                   }
                                   return i++;
                                 }};
  }

  auto
  processor(flow::graph& g, std::string_view label)
  {
    return flow::function_node<int, int>{g, flow::unlimited, [label](int i) {
                                           debug(label, " processing: ", i);
                                           return i;
                                         }};
  }

  auto
  receiver(flow::graph& g)
  {
    return flow::function_node<int>{g, flow::unlimited, [](int i) { debug("-> Received: ", i); }};
  }
}

TEST_CASE("Dynamic join - one connection", "[graph]")
{
  flow::graph g;
  auto src = input_source(g);
  auto f1 = processor(g, "f1");

  dynamic_join_node dynamic_join{g, [](int i) { return i; }};
  auto print_number = receiver(g);

  nodes(src)->nodes(f1)->nodes(dynamic_join)->nodes(print_number);

  src.activate();
  g.wait_for_all();
}

TEST_CASE("Dynamic join - two connections", "[graph]")
{
  flow::graph g;

  auto src = input_source(g);
  auto f1 = processor(g, "f1");
  auto f2 = processor(g, "f2");

  dynamic_join_node djoin{g, [](int i) { return i; }};
  auto print_number = receiver(g);

  nodes(src)->nodes(f1, f2)->nodes(djoin)->nodes(print_number);

  src.activate();
  g.wait_for_all();
}

TEST_CASE("Dynamic join - four connections", "[graph]")
{
  flow::graph g;

  auto src = input_source(g);
  auto f1 = processor(g, "f1");
  auto f2 = processor(g, "f2");
  auto f3 = processor(g, "f3");
  auto f4 = processor(g, "f4");

  dynamic_join_node dynamic_join{g, [](int i) { return i; }};
  auto print_number = receiver(g);

  nodes(src)->nodes(f1, f2, f3, f4)->nodes(dynamic_join)->nodes(print_number);

  src.activate();
  g.wait_for_all();
}
