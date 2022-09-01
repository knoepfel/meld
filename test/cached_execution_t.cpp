#include "meld/core/cached_product_stores.hpp"
#include "meld/graph/transition.hpp"
#include "meld/utilities/debug.hpp"
#include "meld/utilities/sized_tuple.hpp"
#include "meld/utilities/sleep_for.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/flow_graph.h"

using namespace meld;
using namespace oneapi::tbb;

namespace {

  template <std::size_t I>
  using arg_t = sized_tuple<product_store_ptr, I>;

  ProductStoreHasher const matcher{};
  using simple_base = flow::function_node<product_store_ptr, product_store_ptr>;
  class simple_node : public simple_base {
  public:
    simple_node(flow::graph& g, std::string const& name) :
      simple_base{g, flow::unlimited, [this, &name](product_store_ptr const& store) {
                    if (decltype(stores_)::const_accessor a; stores_.find(a, store->id())) {
                      // debug('[' + name + ']', " Reusing store with ID: ",  store->id());
                      return a->second->extend(store->message_id());
                    }
                    decltype(stores_)::accessor a;
                    stores_.emplace(a, store->id(), store);
                    debug('[' + name + ']', " Created store with ID: ", store->id());
                    return a->second;
                  }}
    {
    }

  private:
    tbb::concurrent_hash_map<level_id, product_store_ptr> stores_;
  };

  class user_node : public flow::composite_node<arg_t<2ull>, arg_t<1ull>> {
  public:
    explicit user_node(flow::graph& g, std::string const& name) :
      flow::composite_node<arg_t<2ull>, arg_t<1ull>>{g},
      join_{g, matcher, matcher},
      func_{g, flow::unlimited, [this, &name](arg_t<2ull> const& stores) {
              auto& store = most_derived_store(stores);
              if (decltype(stores_)::const_accessor a; stores_.find(a, store->id())) {
                // debug('[' + name + ']', " Reusing store with ID: ",  store->id());
                return a->second->extend(store->message_id());
              }
              decltype(stores_)::accessor a;
              stores_.emplace(a, store->id(), store);
              debug('[' + name + ']', " Created store with ID: ", store->id());
              return a->second;
            }}
    {
      set_external_ports(input_ports_type{input_port<0>(join_), input_port<1>(join_)},
                         output_ports_type{func_});
      make_edge(join_, func_);
    }

  private:
    flow::join_node<arg_t<2ull>, flow::tag_matching> join_;
    flow::function_node<arg_t<2ull>, product_store_ptr> func_;
    tbb::concurrent_hash_map<level_id, product_store_ptr> stores_;
  };

}

TEST_CASE("Cached function calls", "[data model]")
{
  flow::graph g;
  constexpr std::size_t n_runs{1};
  constexpr std::size_t n_subruns{2u};
  constexpr std::size_t n_events{5u};
  std::vector<level_id> levels;
  for (std::size_t i = 0; i != n_runs; ++i) {
    auto run_id = levels.emplace_back(level_id{i});
    debug("Made ID: ", run_id);
    for (std::size_t j = 0; j != n_subruns; ++j) {
      auto subrun_id = levels.emplace_back(run_id.make_child(j));
      debug("Made ID: ", subrun_id);
      for (std::size_t k = 0; k != n_events; ++k) {
        auto event_id = levels.emplace_back(subrun_id.make_child(k));
        debug("Made ID: ", event_id);
      }
    }
  }

  auto it = cbegin(levels);
  auto const end = cend(levels);
  cached_product_stores stores;
  flow::input_node src{g, [&stores, it, end](flow_control& fc) mutable -> product_store_ptr {
                         if (it == end) {
                           fc.stop();
                           return nullptr;
                         }
                         auto store = stores.get_store(*it++, stage::process);
                         if (store->id().depth() == 1ull) {
                           store->add_product("number", 2 * store->id().back());
                         }
                         if (store->id().depth() == 2ull) {
                           store->add_product("another", 3 * store->id().back());
                         }
                         if (store->id().depth() == 3ull) {
                           store->add_product("still", 4 * store->id().back());
                         }
                         return store;
                       }};

  simple_node A1{g, "A1"};
  simple_node A2{g, "A2"};
  simple_node A3{g, "A3"};
  user_node B1{g, "B1"};
  user_node B2{g, "B2"};
  user_node C{g, "C "};

  struct no_output {};
  flow::multifunction_node<product_store_ptr, std::tuple<no_output>> multiplexer{
    g, flow::unlimited, [&](product_store_ptr const& store, auto&) {
      auto const& a1_store = store->store_with("number");
      // debug("Will send: ", a1_store->id(), " with message ID: ", a1_store->message_id());
      A1.try_put(a1_store);
      if (store->id().depth() > 1ull) {
        auto const& b1_store = store->store_with("another");
        // debug("Will send: ", b1_store->id(), " with message ID: ", b1_store->message_id());
        input_port<1>(B1).try_put(b1_store);
      }
      if (store->id().depth() > 2ull) {
        auto const& c_store = store->store_with("still");
        // debug("Will send: ", c_store->id(), " with message ID: ", c_store->message_id());
        input_port<1>(C).try_put(c_store);
      }
    }};

  make_edge(src, multiplexer);
  make_edge(A1, A2);
  make_edge(A2, A3);
  make_edge(A1, input_port<0>(B1));
  make_edge(B1, input_port<1>(B2));
  make_edge(A2, input_port<0>(B2));
  make_edge(B2, input_port<0>(C));
  src.activate();
  g.wait_for_all();
}
