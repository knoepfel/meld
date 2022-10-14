#ifndef meld_source_hpp
#define meld_source_hpp

#include "boost/dll/alias.hpp"
#include "boost/json.hpp"
#include "meld/core/product_store.hpp"

#include <memory>

namespace meld::detail {

  // See note below.
  template <typename T>
  auto make(boost::json::value const& obj)
  {
    if constexpr (requires { T{obj}; }) {
      return std::make_shared<T>(obj);
    }
    else {
      return std::make_shared<T>();
    }
  }

  template <typename T>
  std::function<product_store_ptr()> create_next(boost::json::value const& obj = {})
  {
    // N.B. Because we are initializing an std::function object with a lambda, the lambda
    //      (and therefore its captured values) must be copy-constructible.  This means
    //      that make<T>(obj) must return a copy-constructible object.  Because we do not
    //      know if a user's provided source class is copyable, we create the object on
    //      the heap, and capture a shared pointer to the object.  This also ensures that
    //      the source object is created only once, thus avoiding potential errors in the
    //      implementations of the source class' copy/move constructors (e.g. if the
    //      source is caching an iterator).
    return [t = make<T>(obj)] { return t->next(); };
  }

  using source_creator_t = std::function<product_store_ptr()>(boost::json::value const&);
}

#define DEFINE_SOURCE(source) BOOST_DLL_ALIAS(meld::detail::create_next<source>, create_source)

#endif /* meld_source_hpp */
