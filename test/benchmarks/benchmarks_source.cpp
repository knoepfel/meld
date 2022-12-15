// ===================================================================
// This source creates 1M events.
// ===================================================================

#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"
#include "meld/source.hpp"

#include "spdlog/spdlog.h"

namespace test {
  class benchmarks_source {
  public:
    benchmarks_source(boost::json::object const& config) :
      max_{value_to<std::size_t>(config.at("n_events"))}
    {
      spdlog::info("Processing {} events", max_);
    }

    meld::product_store_ptr next()
    {
      using meld::level_id;
      if (counter_ == 0) {
        ++counter_;
        return make_product_store(level_id::base());
      }
      if (counter_ == max_ + 1) {
        return nullptr;
      }
      ++counter_;

      if ((counter_ - 1) % (max_ / 10) == 0) {
        spdlog::debug("Reached {} events", counter_ - 1);
      }

      auto store = make_product_store(level_id::base().make_child(counter_ - 1));
      store->add_product("id", store->id());
      return store;
    }

  private:
    std::size_t max_;
    std::size_t counter_{};
  };
}

DEFINE_SOURCE(test::benchmarks_source)
