#ifndef meld_source_hpp
#define meld_source_hpp

#include "boost/dll/alias.hpp"

#include "meld/configuration.hpp"
#include "meld/model/product_store.hpp"

#include <memory>

namespace meld::detail {

  // See note below.
  template <typename T>
  auto make(configuration const& config)
  {
    if constexpr (requires { T{config}; }) {
      return std::make_shared<T>(config);
    }
    else {
      return std::make_shared<T>();
    }
  }

  template <typename T>
  std::function<product_store_ptr()> create_next(configuration const& config = {})
  {
    // N.B. Because we are initializing an std::function object with a lambda, the lambda
    //      (and therefore its captured values) must be copy-constructible.  This means
    //      that make<T>(config) must return a copy-constructible object.  Because we do not
    //      know if a user's provided source class is copyable, we create the object on
    //      the heap, and capture a shared pointer to the object.  This also ensures that
    //      the source object is created only once, thus avoiding potential errors in the
    //      implementations of the source class' copy/move constructors (e.g. if the
    //      source is caching an iterator).
    return [t = make<T>(config)] { return t->next(); };
  }

  using source_creator_t = std::function<product_store_ptr()>(configuration const&);
}

#define DEFINE_SOURCE(source) BOOST_DLL_ALIAS(meld::detail::create_next<source>, create_source)

#endif /* meld_source_hpp */
