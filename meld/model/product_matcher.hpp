#ifndef meld_model_product_matcher_hpp
#define meld_model_product_matcher_hpp

// =======================================================================================
// The full specification of a matcher looks like:
//
//   path/node_name:product_name
//
//
// =======================================================================================

#include "meld/model/fwd.hpp"

#include <array>
#include <string>

namespace meld {
  class product_matcher {
  public:
    explicit product_matcher(std::string matcher_spec);
    bool matches(product_store_const_ptr const& store) const;
    std::string const& level_path() const noexcept { return level_path_; }
    std::string const& module_name() const noexcept { return module_name_; }
    std::string const& node_name() const noexcept { return node_name_; }
    std::string const& product_name() const noexcept { return product_name_; }

    std::string encode() const;

  private:
    explicit product_matcher(std::array<std::string, 4u> fields);
    std::string level_path_;
    std::string module_name_;
    std::string node_name_;
    std::string product_name_;
  };
}

#endif /* meld_model_product_matcher_hpp */

// Local Variables:
// mode: c++
// End:
