#ifndef meld_core_data_level_hpp
#define meld_core_data_level_hpp

#include "meld/graph/data_node.hpp"
#include "meld/utilities/string_literal.hpp"

#include <algorithm>
#include <string_view>

namespace meld {

  template <string_literal Name, typename Parent = root_node_t>
  class data_level : public data_node {
  public:
    static constexpr std::string_view
    name()
    {
      return Name;
    }
    using parent_type = Parent;

    data_level(Parent* parent, std::size_t i);
    Parent* parent() noexcept;
    Parent const* parent() const noexcept;

    // Maybe make const-qualified version of this?
    template <typename Child>
    std::shared_ptr<Child> make_child(std::size_t i);

  private:
    Parent* parent_;
  };

  // Implementation below.

  template <string_literal Name, typename Parent>
  data_level<Name, Parent>::data_level(Parent* parent, std::size_t i) :
    data_node{parent->id(), i, name()}, parent_{parent}
  {
  }

  template <string_literal Name, typename Parent>
  Parent*
  data_level<Name, Parent>::parent() noexcept
  {
    if constexpr (std::is_same_v<Parent, root_node_t>) {
      return nullptr;
    }
    else {
      return parent_;
    }
  }
  template <string_literal Name, typename Parent>
  Parent const*
  data_level<Name, Parent>::parent() const noexcept
  {
    if constexpr (std::is_same_v<Parent, root_node_t>) {
      return nullptr;
    }
    else {
      return parent_;
    }
  }

  template <string_literal Name, typename Parent>
  template <typename Child>
  std::shared_ptr<Child>
  data_level<Name, Parent>::make_child(std::size_t i)
  {
    return std::make_shared<Child>(this, i);
  }

}

#endif /* meld_core_data_level_hpp */
