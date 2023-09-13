#include "meld/core/filter/filter_impl.hpp"
#include "meld/core/filter/result_collector.hpp"
#include "meld/core/framework_graph.hpp"
#include "meld/core/message.hpp"
#include "meld/model/level_hierarchy.hpp"
#include "meld/model/product_store.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/concurrent_vector.h"
#include "oneapi/tbb/flow_graph.h"

using namespace meld;
using namespace oneapi::tbb;

namespace {
  class source {
  public:
    source()
    {
      auto& base = levels_.emplace_back(level_id::base_ptr());
      auto& run = levels_.emplace_back(base->make_child(0, "run"));
      auto& subrun = levels_.emplace_back(run->make_child(0, "subrun"));
      levels_.emplace_back(subrun->make_child(0, "event"));
      it_ = begin(levels_);
    }

    // Not copyable due to iterator it_
    source(source const&) = delete;
    source& operator=(source const&) = delete;

    product_store_ptr next(cached_product_stores& stores)
    {
      if (it_ == levels_.end()) {
        return nullptr;
      }

      auto store = stores.get_store(*it_++);
      store->add_product("id", *store->id());
      return store;
    }

  private:
    std::vector<level_id_ptr> levels_;
    std::vector<level_id_ptr>::const_iterator it_;
  };

  void check_two_ids(level_id const& parent_id, level_id const& id)
  {
    CHECK(id.depth() == 3ull);
    CHECK(parent_id.depth() == 3ull);
    CHECK(parent_id.hash() == id.parent()->hash());
  }

  void check_three_ids(level_id const& grandparent_id,
                       level_id const& parent_id,
                       level_id const& id)
  {
    CHECK(id.depth() == 3ull);
    CHECK(parent_id.depth() == 2ull);
    CHECK(grandparent_id.depth() == 1ull);

    CHECK(grandparent_id.hash() == parent_id.parent()->hash());
    CHECK(parent_id.hash() == id.parent()->hash());
    CHECK(grandparent_id.hash() == id.parent()->parent()->hash());
  }
}

TEST_CASE("Testing domains", "[data model]")
{
  source src;
  framework_graph g{[&src](cached_product_stores& stores) mutable { return src.next(stores); }, 1};
  g.with(check_two_ids).monitor("id"_in("subrun"), "id").for_each("event");
  g.with(check_three_ids).monitor("id"_in("run"), "id"_in("subrun"), "id").for_each("event");
  CHECK_THROWS(g.execute("allowed_domains_t.gv")); // Due to duplicate product names

  CHECK(g.execution_counts("check_two_ids") == 0ull);
  CHECK(g.execution_counts("check_three_ids") == 0ull);
}
