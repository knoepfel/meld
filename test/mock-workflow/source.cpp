#include "meld/source.hpp"
#include "meld/model/level_id.hpp"
#include "meld/model/product_store.hpp"

#include "spdlog/spdlog.h"

namespace meld::test {
  class source {
  public:
    source(meld::configuration const& config) : max_{config.get<std::size_t>("n_events")}
    {
      spdlog::info("Processing {} events", max_);
    }

    meld::product_store_ptr next()
    {
      using meld::level_id;
      if (counter_ == 0) {
        ++counter_;
        return meld::product_store::base();
      }
      if (counter_ == max_ + 1) {
        return nullptr;
      }
      ++counter_;

      if (max_ > 10 && (counter_ - 1) % (max_ / 10) == 0) {
        spdlog::debug("Reached {} events", counter_ - 1);
      }

      auto store = meld::product_store::base()->make_child(counter_ - 1, "event");
      store->add_product("id", *store->id());
      return store;
    }

  private:
    std::size_t max_;
    std::size_t counter_{};
  };
}

DEFINE_SOURCE(meld::test::source)
