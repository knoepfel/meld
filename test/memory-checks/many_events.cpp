#include "meld/core/framework_graph.hpp"
#include "meld/model/product_store.hpp"
#include "test/products_for_output.hpp"

using namespace meld;
using namespace meld::concurrency;

namespace {
  unsigned pass_on(unsigned number) { return number; }
}

int main()
{
  constexpr auto max_events{100'000u};
  // constexpr auto max_events{1'000'000u};
  // spdlog::flush_on(spdlog::level::trace);

  framework_graph g{[i = 0u]() mutable -> product_store_ptr {
    if (i == max_events + 1) { // + 1 is for initial product store
      return nullptr;
    }
    if (i == 0u) {
      ++i;
      return product_store::base();
    }

    auto store = product_store::base()->make_child(i, "event", "Source");
    store->add_product("number", i);
    ++i;
    return store;
  }};

  g.with(pass_on).using_concurrency(unlimited).transform("number").to("different");
  g.execute();
}
