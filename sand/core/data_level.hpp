#ifndef sand_core_data_level_hpp
#define sand_core_data_level_hpp

#include "sand/core/node.hpp"

namespace sand {

  template <typename Parent = null_node_t>
  class data_level : public node {
  public:
    data_level(Parent const* parent, std::size_t i);
    ~data_level() final;
    Parent const* parent() const noexcept;

    template <typename Child>
    std::shared_ptr<Child> make_child(std::size_t i) const;

  private:
    Parent const* parent_;
  };

  // Implementation below.

  template <typename Parent>
  data_level<Parent>::data_level(Parent const* parent, std::size_t i) :
    node{parent->id(), i}, parent_{parent}
  {
  }

  template <typename Parent>
  data_level<Parent>::~data_level() = default;

  template <typename Parent>
  Parent const*
  data_level<Parent>::parent() const noexcept
  {
    if constexpr (std::is_same_v<Parent, null_node_t>) {
      return nullptr;
    }
    else {
      return parent_;
    }
  }

  template <typename Parent>
  template <typename Child>
  std::shared_ptr<Child>
  data_level<Parent>::make_child(std::size_t i) const
  {
    return node::make_child<Child>(this, i);
  }

}

#endif /* sand_core_data_level_hpp */
