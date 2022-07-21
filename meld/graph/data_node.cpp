#include "meld/graph/data_node.hpp"

#include <ostream>

namespace meld {
  data_node::data_node(level_id id, std::string_view name) : id_{std::move(id)}, name_{name} {}
  data_node::data_node(level_id const& parent_id, std::size_t new_id, std::string_view name) :
    id_{parent_id.make_child(new_id)}, name_{name}
  {
  }

  level_id const&
  data_node::id() const noexcept
  {
    return id_;
  }

  std::string_view
  data_node::level_name() const noexcept
  {
    return name_;
  }

  transition_type
  ttype_for(transition_message const& msg)
  {
    auto const& [tr, node_ptr] = msg;
    return transition_type{node_ptr->level_name(), tr.second};
  }

  root_node_t::root_node_t() : data_node{{}, "root-node"} {}

  std::shared_ptr<root_node_t> root_node{std::make_shared<root_node_t>()};

  std::ostream&
  operator<<(std::ostream& os, data_node const& n)
  {
    return os << n.id();
  }

}
