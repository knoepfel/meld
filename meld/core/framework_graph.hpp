#ifndef meld_core_framework_graph_hpp
#define meld_core_framework_graph_hpp

#include "meld/core/declared_reduction.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/core/product_store.hpp"
#include "meld/graph/dynamic_join_node.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

  class framework_graph {
    using source_node = tbb::flow::input_node<product_store_ptr>;
    using dynamic_join = dynamic_join_node<product_store_ptr, ProductStoreHasher>;

  public:
    using user_function_node = tbb::flow::function_node<product_store_ptr, product_store_ptr>;
    using reduction_node =
      tbb::flow::multifunction_node<product_store_ptr, std::tuple<product_store_ptr>>;
    struct run_once_t {};
    static constexpr run_once_t run_once{};

    explicit framework_graph(run_once_t, product_store_ptr store);

    template <typename FT>
    explicit framework_graph(FT ft) : src_{graph_, std::move(ft)}
    {
    }

    void merge(declared_transforms&& user_fcns, declared_reductions&& user_reductions);
    void finalize_and_run();

  private:
    user_function_node node_for(declared_transform_ptr& p);
    reduction_node node_for(declared_reduction_ptr& p);

    void run();
    void finalize();

    tbb::flow::sender<product_store_ptr>& sender_for(std::string const& name);
    tbb::flow::receiver<product_store_ptr>& receiver_for(std::string const& name);
    dynamic_join& join_for(std::string const& name);

    std::map<std::string, std::vector<std::string>> produced_products() const;
    std::map<std::string, std::vector<std::string>> consumed_products() const;

    tbb::flow::graph graph_{};
    declared_transforms funcs_{};
    declared_reductions reductions_{};
    std::map<std::string, dynamic_join> joins_{};
    std::map<std::string, user_function_node> transform_nodes_{};
    std::map<std::string, reduction_node> reduction_nodes_{};
    tbb::flow::input_node<product_store_ptr> src_;
  };
}

#endif /* meld_core_framework_graph_hpp */
