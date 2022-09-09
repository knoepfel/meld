#ifndef meld_core_framework_graph_hpp
#define meld_core_framework_graph_hpp

#include "meld/core/declared_reduction.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/core/product_store.hpp"
#include "meld/core/user_functions.hpp"
#include "meld/graph/dynamic_join_node.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

  using dynamic_join = dynamic_join_node<product_store_ptr, ProductStoreHasher>;
  class framework_graph {
    using source_node = tbb::flow::input_node<product_store_ptr>;

  public:
    struct run_once_t {};
    static constexpr run_once_t run_once{};

    explicit framework_graph(run_once_t, product_store_ptr store);

    template <typename FT>
    explicit framework_graph(FT ft) :
      src_{graph_, std::move(ft)},
      multiplexer_{graph_,
                   tbb::flow::serial, // FIXME: Change this when necessary
                   [this](product_store_ptr const& store) -> tbb::flow::continue_msg {
                     return multiplex(store);
                   }}

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

    tbb::flow::continue_msg multiplex(product_store_ptr const& ptr);

    tbb::flow::graph graph_{};
    declared_transforms transforms_{};
    declared_reductions reductions_{};
    std::map<std::string, tbb::flow::receiver<product_store_ptr>*> head_nodes_;
    tbb::flow::input_node<product_store_ptr> src_;
    tbb::flow::function_node<product_store_ptr> multiplexer_;
    std::map<level_id, std::set<tbb::flow::receiver<product_store_ptr>*>> flushes_required_;
  };
}

#endif /* meld_core_framework_graph_hpp */
