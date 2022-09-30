#ifndef test_products_for_output_hpp
#define test_products_for_output_hpp

#include "meld/core/product_store.hpp"
#include "meld/utilities/debug.hpp"

#include <sstream>

namespace meld::test {
  struct products_for_output {
    void save(product_store const& store) const
    {
      std::ostringstream oss;
      oss << "Saving data for " << store.id() << " from " << store.source() << '\n';
      for (auto const& [product_name, _] : store) {
        oss << " -> Product name: " << product_name << '\n';
      }
      debug(oss.str());
    }
  };

}

#endif /* test_products_for_output_hpp */
