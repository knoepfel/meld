// ===================================================================
// This source creates 1M events.
// ===================================================================

#include "meld/model/level_hierarchy.hpp"
#include "meld/model/product_store.hpp"
#include "meld/model/transition.hpp"
#include "meld/source.hpp"

#include "spdlog/spdlog.h"

namespace test {
  class benchmarks_source {
  public:
    benchmarks_source(meld::configuration const& config) : max_{config.get<std::size_t>("n_events")}
    {
      spdlog::info("Processing {} events", max_);
    }

    meld::product_store_ptr next()
    {
      using meld::level_id;
      if (counter_ == 0) {
        ++counter_;
        return factory_.make(level_id::base());
      }
      if (counter_ == max_ + 1) {
        return nullptr;
      }
      ++counter_;

      if ((counter_ - 1) % (max_ / 10) == 0) {
        spdlog::debug("Reached {} events", counter_ - 1);
      }

      auto store = factory_.make(level_id::base().make_child(counter_ - 1));
      store->add_product("id", store->id());
      return store;
    }

  private:
    meld::level_hierarchy org_;
    meld::product_store_factory factory_{org_.make_factory("event")};
    std::size_t max_;
    std::size_t counter_{};
  };
}

DEFINE_SOURCE(test::benchmarks_source)
