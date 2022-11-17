#ifndef test_benchmarks_diamond_source_hpp
#define test_benchmarks_diamond_source_hpp

// ===================================================================
// This source creates 1M events.
// ===================================================================

#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"
#include "meld/utilities/debug.hpp"

namespace test {
  inline constexpr std::size_t n_events{1'000'000};

  class diamond_source {
  public:
    meld::product_store_ptr next()
    {
      if (counter_ > 2 * n_events) {
        return nullptr;
      }
      ++counter_;

      if (counter_ % (2 * n_events / 10) == 0) {
        meld::debug("Reached ", counter_ / 2, " events");
      }

      if (counter_ % 2 == 0) {
        meld::level_id const id{counter_ / 2 - 1, 0};
        return meld::make_product_store(id, {}, meld::stage::flush);
      }
      meld::level_id const id{counter_ / 2};
      auto store = meld::make_product_store(id);
      store->add_product("id", store->id());
      return store;
    }

  private:
    std::size_t counter_{};
  };
}

#endif /* test_benchmarks_diamond_source_hpp */
