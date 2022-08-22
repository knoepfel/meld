#ifndef meld_core_framework_graph_hpp
#define meld_core_framework_graph_hpp

#include "meld/core/declared_function.hpp"
#include "meld/core/product_store.hpp"
#include "meld/graph/dynamic_join_node.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

  class framework_graph {
    using source_node = tbb::flow::input_node<product_store_ptr>;
    using user_function_node = tbb::flow::function_node<product_store_ptr, product_store_ptr>;
    using dynamic_join = dynamic_join_node<product_store_ptr, ProductStoreHasher>;

    std::string
    join_name_for(std::string const& node_name)
    {
      return "[join for " + node_name + ']';
    }

  public:
    struct run_once_t {};
    static constexpr run_once_t run_once{};

    explicit framework_graph(run_once_t, product_store_ptr store) :
      src_{graph_, [store, executed = false](auto& fc) mutable -> product_store_ptr {
             if (executed) {
               fc.stop();
               return {};
             }
             executed = true;
             return store;
           }}
    {
    }

    template <typename FT>
    explicit framework_graph(FT ft) : src_{graph_, std::move(ft)}
    {
    }

    void
    merge(declared_functions&& user_fcns)
    {
      funcs_.merge(std::move(user_fcns));
    }

    void
    finalize_and_run()
    {
      finalize();
      run();
    }

    void
    run()
    {
      src_.activate();
      graph_.wait_for_all();
    }

    void
    finalize()
    {
      for (auto const& [name, p] : funcs_) {
        joins_.try_emplace(join_name_for(name), dynamic_join{graph_, ProductStoreHasher{}});
        // N.B. Capturing 'p' by reference is supported as of C++20.  Apple-Clang 13.1.6 isn't yet up to snuff.
        nodes_.try_emplace(
          name, user_function_node{graph_, p->concurrency(), [ptr = p.get()](auto store) {
                                     ptr->invoke(*store);
                                     return store;
                                   }});
      }

      std::set<std::pair<std::string, std::string>> existing_edges;
      auto const available_products = produced_products();
      for (auto const& [fcn_name, product_labels] : consumed_products()) {
        auto& fcn_node = nodes_.at(fcn_name);
        for (auto const& label : product_labels) {
          if (auto it = available_products.find(label); it != cend(available_products)) {
            // FIXME:  THIS LOOKUP ASSUMES ONLY ONE FUNCTION PROVIDES THE PRODUCT
            auto const& candidate_fcns = it->second;
            auto const& sender_name = candidate_fcns[0];
            if (existing_edges.contains({sender_name, fcn_name})) {
              continue;
            }
            make_edge(nodes_.at(sender_name), joins_.at(join_name_for(fcn_name)));
            make_edge(joins_.at(join_name_for(fcn_name)), fcn_node);
            existing_edges.insert({sender_name, fcn_name});
          }
          else {
            if (existing_edges.contains({"[source]", fcn_name})) {
              continue;
            }
            make_edge(src_, joins_.at(join_name_for(fcn_name)));
            make_edge(joins_.at(join_name_for(fcn_name)), fcn_node);
            existing_edges.insert({"[source]", fcn_name});
          }
        }
      }
    }

  private:
    std::map<std::string, std::vector<std::string>>
    produced_products() const
    {
      // [product name] <=> [function names]
      std::map<std::string, std::vector<std::string>> result;
      for (auto const& [_, function] : funcs_) {
        for (auto const& label : function->output()) {
          result[label].push_back(function->name());
        }
      }
      return result;
    }

    std::map<std::string, std::vector<std::string>>
    consumed_products() const
    {
      // [function name] <=> [product names]
      std::map<std::string, std::vector<std::string>> result;
      for (auto const& [_, function] : funcs_) {
        result.try_emplace(function->name(), function->input());
      }
      return result;
    }

    tbb::flow::graph graph_{};
    declared_functions funcs_{};
    std::map<std::string, dynamic_join> joins_{};
    std::map<std::string, user_function_node> nodes_{};
    tbb::flow::input_node<product_store_ptr> src_;
  };
}

#endif /* meld_core_framework_graph_hpp */
