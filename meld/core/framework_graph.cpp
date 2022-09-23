#include "meld/core/framework_graph.hpp"

#include "meld/core/edge_maker.hpp"
#include "meld/core/product_store.hpp"

namespace meld {
  framework_graph::framework_graph(run_once_t, product_store_ptr store) :
    framework_graph{[store, executed = false]() mutable -> product_store_ptr {
      if (executed) {
        return nullptr;
      }
      executed = true;
      return store;
    }}
  {
  }

  void framework_graph::execute()
  {
    finalize();
    run();
  }

  void framework_graph::run()
  {
    src_.activate();
    graph_.wait_for_all();
  }

  void framework_graph::finalize()
  {
    edge_maker make_edges{"framework-graph.gv", transforms_, reductions_};
    make_edges(multiplexer_,
               consumers{transforms_, {.shape = "ellipse"}},
               consumers{reductions_, {.arrowtail = "dot", .shape = "ellipse"}},
               consumers{splitters_, {.shape = "trapezium"}});
    make_edge(src_, multiplexer_);
  }
}
