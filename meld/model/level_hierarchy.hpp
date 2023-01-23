#ifndef meld_model_level_hierarchy_hpp
#define meld_model_level_hierarchy_hpp

#include "meld/model/fwd.hpp"
#include "meld/model/product_store_factory.hpp"

#include <concepts>
#include <iosfwd>
#include <set>
#include <utility>
#include <vector>

namespace meld {

  class level_hierarchy {
  public:
    product_store_factory make_factory(std::vector<std::string> level_names);
    product_store_factory make_factory(std::convertible_to<std::string> auto&&... level_names)
    {
      return make_factory({std::forward<decltype(level_names)>(level_names)...});
    }

    std::size_t index(std::string const& name) const;
    std::string const& level_name(std::size_t const name_index) const;

    std::vector<std::vector<std::string>> orders() const;

    void print(std::ostream& os) const;

  private:
    level_order add_order(std::vector<std::string> const& levels);
    void print(std::ostream& os, level_order const& h) const;

    struct level_entry {
      std::string name;
      std::size_t parent_id;
    };

    std::vector<level_entry> levels_;
    std::set<level_order> orders_;
  };

}

#endif /* meld_model_level_hierarchy_hpp */
