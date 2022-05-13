#ifndef meld_core_data_level_hpp
#define meld_core_data_level_hpp

#include "meld/graph/node.hpp"

#include <algorithm>
#include <string_view>

namespace meld {

  namespace detail {
    template <size_t N>
    struct string_literal {
      constexpr string_literal(char const (&str)[N]) { std::copy_n(str, N, value); }
      constexpr operator std::string_view() const { return value; }
      char value[N];
    };
  }

  template <detail::string_literal Name, typename Parent = root_node_t>
  class data_level : public node {
  public:
    static constexpr std::string_view
    name()
    {
      return Name;
    }
    using parent_type = Parent;

    data_level(Parent* parent, std::size_t i);
    ~data_level() final;
    Parent* parent() noexcept;
    Parent const* parent() const noexcept;

    // Maybe make const-qualified version of this?
    template <typename Child>
    std::shared_ptr<Child> make_child(std::size_t i);

  private:
    std::string_view level_name() const final;
    Parent* parent_;
  };

  // Implementation below.

  template <detail::string_literal Name, typename Parent>
  data_level<Name, Parent>::data_level(Parent* parent, std::size_t i) :
    node{parent->id(), i}, parent_{parent}
  {
  }

  template <detail::string_literal Name, typename Parent>
  data_level<Name, Parent>::~data_level() = default;

  template <detail::string_literal Name, typename Parent>
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
  template <detail::string_literal Name, typename Parent>
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

  template <detail::string_literal Name, typename Parent>
  template <typename Child>
  std::shared_ptr<Child>
  data_level<Name, Parent>::make_child(std::size_t i)
  {
    return std::make_shared<Child>(this, i);
  }

  template <detail::string_literal Name, typename Parent>
  std::string_view
  data_level<Name, Parent>::level_name() const
  {
    return name();
  }

}

#endif /* meld_core_data_level_hpp */
