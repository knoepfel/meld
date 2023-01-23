#include "meld/source.hpp"
#include "meld/core/cached_product_stores.hpp"
#include "meld/model/level_hierarchy.hpp"
#include "meld/model/product_store.hpp"

namespace {
  class number_generator {
  public:
    number_generator(meld::configuration const& config) : n_{config.get<int>("max_numbers")} {}

    meld::product_store_ptr next()
    {
      if (current_ == 0) {
        ++current_;
        return stores_.get_empty_store(meld::level_id::base());
      }
      if (current_++ == n_ + 1) {
        return nullptr;
      }

      meld::level_id const id{static_cast<unsigned int>(current_)};
      auto store = stores_.get_empty_store(id);
      store->add_product("i", current_);
      store->add_product("j", -current_);
      return store;
    }

  private:
    int n_;
    int current_{};
    meld::level_hierarchy org_;
    meld::cached_product_stores stores_{org_.make_factory("event")};
  };
}

DEFINE_SOURCE(number_generator)
