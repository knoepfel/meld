#ifndef meld_core_framework_graph_hpp
#define meld_core_framework_graph_hpp

#include "meld/core/declared_reduction.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/core/product_store.hpp"
#include "meld/core/user_functions.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

  class framework_graph {
  public:
    struct run_once_t {};
    static constexpr run_once_t run_once{};

    explicit framework_graph(run_once_t, product_store_ptr store);

    template <typename FT>
    explicit framework_graph(FT ft) :
      src_{graph_,
           [this, user_function = std::move(ft)](tbb::flow_control& fc) mutable -> message {
             auto store = user_function();
             if (not store) {
               fc.stop();
               return {};
             }

             ++calls_;
             if (store->is_flush()) {
               debug("Parent ID: ", store->parent()->id());
               auto it = original_message_ids_.find(store->parent()->id());
               assert(it != cend(original_message_ids_));
               std::size_t const original_message_id = it->second;
               return {store, calls_, original_message_id};
             }

             // FIXME: Need to find way to cleanup the original message-IDs map.
             original_message_ids_.try_emplace(store->id(), calls_);
             return {store, calls_};
           }},
      multiplexer_{graph_,
                   tbb::flow::unlimited,
                   [this](message const& msg) -> tbb::flow::continue_msg { return multiplex(msg); }}

    {
    }

    void execute();

    auto
    make_component()
    {
      return user_functions<void_tag>{graph_, transforms_, reductions_};
    }

    template <typename T, typename... Args>
    auto
    make_component(Args&&... args)
    {
      return user_functions<T>{graph_, transforms_, reductions_, std::forward<Args>(args)...};
    }

  private:
    void run();
    void finalize();

    tbb::flow::continue_msg multiplex(message const& msg);

    tbb::flow::graph graph_{};
    declared_transforms transforms_{};
    declared_reductions reductions_{};
    std::map<std::string, tbb::flow::receiver<message>*> head_nodes_;
    tbb::flow::input_node<message> src_;
    tbb::flow::function_node<message> multiplexer_;
    tbb::concurrent_hash_map<level_id, std::set<tbb::flow::receiver<message>*>> flushes_required_;
    std::map<level_id, std::size_t> original_message_ids_;
    std::size_t calls_{};
  };
}

#endif /* meld_core_framework_graph_hpp */
