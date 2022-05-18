#include "meld/utilities/thread_counter.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb.h"

#include <chrono>
#include <thread>

using namespace meld;
using namespace oneapi::tbb;
using namespace std::chrono_literals;

TEST_CASE("Thread counter exception", "[multithreading]")
{
  thread_counter::counter_type counter{};
  CHECK_THROWS_WITH((thread_counter{counter, 0u}), Catch::Contains("Too many threads encountered"));
}

TEST_CASE("Thread counter in flow graph", "[multithreading]")
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
  thread_counter::counter_type unlimited_counter{};
  flow::function_node<unsigned int, unsigned int> unlimited_node{
    g, flow::unlimited, [&unlimited_counter](unsigned int const i) {
      thread_counter c{unlimited_counter, -1u};
      std::this_thread::sleep_for(5ms);
      return i;
    }};
  thread_counter::counter_type serial_counter{};
  flow::function_node<unsigned int, unsigned int> serial_node{
    g, flow::serial, [&serial_counter](unsigned int const i) {
      thread_counter c{serial_counter};
      std::this_thread::sleep_for(10ms);
      return i;
    }};
  thread_counter::counter_type max_counter{};
  flow::function_node<unsigned int, unsigned int> max_node{
    g, 4, [&max_counter](unsigned int const i) {
      thread_counter c{max_counter, 4};
      std::this_thread::sleep_for(10ms);
      return i;
    }};
  make_edge(src, unlimited_node);
  make_edge(src, serial_node);
  make_edge(src, max_node);
  src.activate();
  g.wait_for_all();
}
