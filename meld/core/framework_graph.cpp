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

  framework_graph::framework_graph(std::function<product_store_ptr()> f) :
    src_{graph_,
         [this, user_function = move(f)](tbb::flow_control& fc) mutable -> message {
           auto store = user_function();
           if (not store) {
             fc.stop();
             return {};
           }

           ++calls_;
           if (store->is_flush()) {
             // Original message ID no longer needed after this message.
             auto h = original_message_ids_.extract(store->id().parent());
             assert(h);
             std::size_t const original_message_id = h.mapped();
             return {store, calls_, original_message_id};
           }

           // debug("Inserting message ID for ", store->id(), ": ", calls_);
           original_message_ids_.try_emplace(store->id(), calls_);
           return {store, calls_};
         }},
    multiplexer_{graph_}
  {
  }

  void framework_graph::execute(std::string const& dot_file_name)
  {
    finalize(dot_file_name);
    run();
  }

  void framework_graph::run()
  {
    src_.activate();
    graph_.wait_for_all();
  }

  void framework_graph::finalize(std::string const& dot_file_name)
  {
    edge_maker make_edges{dot_file_name, outputs_, transforms_, reductions_};
    make_edges(src_,
               multiplexer_,
               consumers{transforms_, {.shape = "ellipse"}},
               consumers{reductions_, {.arrowtail = "dot", .shape = "ellipse"}},
               consumers{splitters_, {.shape = "trapezium"}});
  }
}
