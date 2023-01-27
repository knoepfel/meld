// =======================================================================================
// This plugin contains *both* a source and a monitor.  This is not normally what anyone
// would want to do.  But Boost's DLL support is robust enough to handle this
// circumstance.
//
// The goal is to test whether the maximum allowed parallelism (as specified by either the
// meld command line, or configuration) agrees with what is expected.
// =======================================================================================

#include "meld/concurrency.hpp"
#include "meld/model/product_store.hpp"
#include "meld/module.hpp"
#include "meld/source.hpp"

#include <cassert>

using namespace meld;

namespace {
  class send_parallelism {
  public:
    meld::product_store_ptr next()
    {
      if (executed_) {
        return nullptr;
      }
      executed_ = true;
      auto store = product_store::base();
      store->add_product("max_parallelism", concurrency::max_allowed_parallelism::active_value());
      return store;
    }

  private:
    bool executed_{false};
  };

  void verify_expected(std::size_t actual, std::size_t expected) { assert(actual == expected); }
}

// Framework glue
DEFINE_SOURCE(send_parallelism)
DEFINE_MODULE(m, config)
{
  m.declare_monitor(verify_expected)
    .input(react_to("max_parallelism"), use(config.get<std::size_t>("expected_parallelism")));
}
