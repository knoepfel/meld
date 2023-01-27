#include "meld/source.hpp"
#include "meld/model/product_store.hpp"

namespace {
  class number_generator {
  public:
    number_generator(meld::configuration const& config) : n_{config.get<int>("max_numbers")} {}

    meld::product_store_ptr next()
    {
      if (current_ == 0) {
        ++current_;
        return meld::product_store::base();
      }
      if (current_++ == n_ + 1) {
        return nullptr;
      }

      auto store = meld::product_store::base()->make_child(current_, "event");
      store->add_product("i", current_);
      store->add_product("j", -current_);
      return store;
    }

  private:
    int n_;
    int current_{};
  };
}

DEFINE_SOURCE(number_generator)
