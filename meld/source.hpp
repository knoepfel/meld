#ifndef meld_source_hpp
#define meld_source_hpp

#include "boost/dll/alias.hpp"
#include "boost/json.hpp"
#include "meld/core/product_store.hpp"

#include <memory>

namespace meld::detail {
  template <typename T>
  T make(boost::json::value const& obj)
  {
    if constexpr (requires { T{obj}; }) {
      return T{obj};
    }
    else {
      return T{};
    }
  }

  template <typename T>
  std::function<product_store_ptr()> create_next(boost::json::value const& obj)
  {
    return [t = make<T>(obj)]() mutable -> product_store_ptr { return t.next(); };
  }

  using source_creator_t = std::function<product_store_ptr()>(boost::json::value const&);
}

#define DEFINE_SOURCE(source) BOOST_DLL_ALIAS(meld::detail::create_next<source>, create_source)

#endif /* meld_source_hpp */
