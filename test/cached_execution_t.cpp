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
  constexpr std::size_t bypass{};

  ProductStoreHasher const matcher{};
  template <std::size_t I>
  using arg_t = sized_tuple<product_store_ptr, I>;

  class user_node : public flow::composite_node<arg_t<3ull>, arg_t<2ull>> {
  public:
    explicit user_node(flow::graph& g, std::string const& name) :
      flow::composite_node<arg_t<3ull>, arg_t<2ull>>{g},
      join_{g, matcher, matcher},
      func_{g,
            flow::unlimited,
            [this, &name](arg_t<2ull> const& stores) -> product_store_ptr {
              auto& store = std::get<0>(stores);
              decltype(data_)::accessor a;
              data_.insert(a, store->id());
              debug(name, " Creating data product for ", store->id());
              sleep_for(0.5s);
              a->second = store;
              debug(name, " Done with data product for ", store->id());
              return store;
            }},
      bypass_{g,
              flow::unlimited,
              [this, &name](product_store_ptr const& store, auto& outputs) -> product_store_ptr {
                decltype(data_)::const_accessor a;
                if (data_.find(a, store->parent()->id())) {
                  debug(name, " Forwarding ", store->id());
                  std::get<0>(outputs).try_put(store->extend(action::process));
                }
                else {
                  // FIXME: This is gross, icky, yucky.  Seriously, this is
                  // the best we can come up with?  A busy loop until the
                  // other thing is ready?  This needs to be fixed...badly.
                  std::get<1>(outputs).try_put(store->extend(action::process));
                }
                return store->extend(action::process);
              }}
    {
      set_external_ports(input_ports_type{bypass_, input_port<0>(join_), input_port<1>(join_)},
                         output_ports_type{output_port<bypass>(bypass_), func_});
      make_edge(join_, func_);
      make_edge(output_port<1>(bypass_), bypass_);
    }

  private:
    flow::join_node<arg_t<2ull>, flow::tag_matching> join_;
    flow::function_node<arg_t<2ull>, product_store_ptr> func_;
    flow::multifunction_node<product_store_ptr, arg_t<2ull>> bypass_;
    tbb::concurrent_hash_map<level_id, product_store_ptr> data_;
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
                         return stores.get_store(*it++, action::process);
                       }};

  user_node run_producer{g, "[Run producer...]"};
  user_node subrun_producer{g, "[Subrun producer]"};
  user_node event_producer{g, "[Event producer.]"};

  struct no_output {};
  flow::multifunction_node<product_store_ptr, std::tuple<no_output>> multiplexer{
    g,
    flow::unlimited,
    [&event_producer, &subrun_producer, &run_producer](product_store_ptr const& store, auto&) {
      if (store->id().depth() == 1ull) {
        debug("[Multiplexer....] Sending ", store->id());
        auto new_store = store->extend(action::process);
        input_port<1ull>(run_producer).try_put(new_store);
        input_port<2ull>(run_producer).try_put(new_store);
      }
      else if (store->id().depth() == 2ull) {
        input_port<1>(subrun_producer).try_put(store->extend(action::process));
        auto new_store = store->extend(action::forward);
        debug("[Multiplexer....] Sending new ", new_store->id());
        input_port<bypass>(run_producer).try_put(new_store);
      }
      else if (store->id().depth() == 3ull) {
        input_port<1>(event_producer).try_put(store->extend(action::process));
        auto new_store = store->extend(action::forward);
        debug("[Multiplexer....] Sending new ", new_store->id());
        input_port<bypass>(subrun_producer).try_put(new_store);
        // input_port<bypass>(run_producer).try_put(new_store); // Maybe add this?
      }
    }};

  std::atomic<unsigned int> run_count = 0;
  flow::function_node run_saver{
    g, flow::unlimited, [&run_count](product_store_ptr const& store) -> flow::continue_msg {
      debug("[Run saver......] Received ", store->id());
      ++run_count;
      return {};
    }};
  std::atomic<unsigned int> subrun_count{};
  flow::function_node subrun_saver{
    g, flow::unlimited, [&subrun_count](product_store_ptr const& store) -> flow::continue_msg {
      debug("[Subrun saver...] Received ", store->id());
      ++subrun_count;
      return {};
    }};
  std::atomic<unsigned int> event_count{};
  flow::function_node event_saver{
    g, flow::unlimited, [&event_count](product_store_ptr const& store) -> flow::continue_msg {
      debug("[Event saver....] Received ", store->id());
      ++event_count;
      return {};
    }};

  make_edge(src, multiplexer);
  make_edge(output_port<1>(run_producer), run_saver);
  make_edge(output_port<bypass>(run_producer), input_port<2>(subrun_producer));
  make_edge(output_port<1>(subrun_producer), subrun_saver);
  make_edge(output_port<bypass>(subrun_producer), input_port<2>(event_producer));
  make_edge(output_port<1>(event_producer), event_saver);
  src.activate();
  g.wait_for_all();
}
