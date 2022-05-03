#include "meld/core/gatekeeper_node.hpp"
#include "meld/core/node.hpp"
#include "meld/utilities/debug.hpp"
#include "oneapi/tbb/flow_graph.h"

#include "catch2/catch.hpp"

using namespace meld;
using namespace tbb;

namespace {
  std::vector<std::string> const level_names{"job", "dataset", "run", "subrun", "event"};

  class test_node : public node {
  public:
    explicit test_node(level_id const& id, std::string level_name) :
      node{id}, name{move(level_name)}
    {
    }

  private:
    std::string_view
    level_name() const final
    {
      return name;
    }
    std::string name;
  };
}

TEST_CASE("Gatekeeper", "[multithreading]")
{
  std::vector const levels{
    "1"_id,
    "1:2"_id,
    "1:4"_id,
    "1:4:1"_id,
    "1:4:2"_id,
    "1:4:3"_id,
    "1:5"_id,
    "1:6"_id,
    "2"_id,
    "2:1"_id,
  };
  auto const tr = transitions_for(levels);

  flow::graph g;
  flow::input_node<transition_message> source{
    g, [it = begin(tr), stop = end(tr)](flow_control& fc) mutable -> transition_message {
      if (it == stop) {
        fc.stop();
        return {};
      }
      auto const& tr = *it++;
      auto const& id = tr.first;
      auto const& level_name = level_names[size(id)];
      return {tr, std::make_shared<test_node>(id, level_name)};
    }};

  gatekeeper_node gatekeeper{g};

  std::atomic<unsigned> setup_calls{0};
  flow::function_node setup{g, flow::unlimited, [&setup_calls](transition_message const& tr) {
                              ++setup_calls;
                              debug("2.  Setting up ", tr.first);
                              return tr;
                            }};
  std::atomic<unsigned> process_calls{0};
  flow::function_node processor{g, flow::unlimited, [&process_calls](transition_message const& tr) {
                                  ++process_calls;
                                  debug("3.  Processing ", tr.first);
                                  return tr;
                                }};

  make_edge(source, input_port<0>(gatekeeper));
  make_edge(output_port<0>(gatekeeper), setup);
  make_edge(output_port<1>(gatekeeper), processor);
  make_edge(setup, input_port<1>(gatekeeper));
  make_edge(processor, input_port<1>(gatekeeper));
  source.activate();
  g.wait_for_all();

  // The '+ 1' is for the job-level calls (id: []) at the beginning and the end.
  CHECK(setup_calls.load() == size(levels) + 1);
  CHECK(process_calls.load() == size(levels) + 1);
}
