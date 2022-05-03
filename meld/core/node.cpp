#include "meld/core/node.hpp"

#include <ostream>

namespace meld {
  node::node(level_id id) : id_{std::move(id)} {}
  node::node(level_id const& parent_id, std::size_t new_id) : id_{parent_id.make_child(new_id)} {}

  node::~node() = default;

  level_id const&
  node::id() const noexcept
  {
    return id_;
  }

  transition_type
  ttype_for(transition_message const& msg)
  {
    auto const& [tr, node_ptr] = msg;
    return transition_type{node_ptr->level_name(), tr.second};
  }

  null_node_t::null_node_t() : node{{}} {}
  null_node_t::~null_node_t() = default;
  std::string_view
  null_node_t::level_name() const
  {
    return "null-node";
  }

  null_node_t null_node{};

  std::ostream&
  operator<<(std::ostream& os, node const& n)
  {
    return os << n.id();
  }

}
