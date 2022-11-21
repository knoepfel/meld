#ifndef test_benchmarks_diamond_source_hpp
#define test_benchmarks_diamond_source_hpp

// ===================================================================
// This source creates 1M events.
// ===================================================================

#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"

#include "spdlog/spdlog.h"

using meld::level_id;

namespace test {
  inline constexpr std::size_t n_events{1'000'000};

  class diamond_source {
  public:
    meld::product_store_ptr next()
    {
      if (counter_ == 0) {
        ++counter_;
        return make_product_store(level_id::base());
      }
      if (counter_ == n_events + 1) {
        return nullptr;
      }
      ++counter_;

      if ((counter_ - 1) % (n_events / 10) == 0) {
        spdlog::debug("Reached {} events", counter_ - 1);
      }

      auto store = make_product_store(level_id::base().make_child(counter_ - 1));
      store->add_product("id", store->id());
      return store;
    }

  private:
    std::size_t counter_{};
  };
}

#endif /* test_benchmarks_diamond_source_hpp */
