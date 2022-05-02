#include "meld/core/node.hpp"

#include <ostream>

namespace meld {
  namespace {
    std::vector<size_t>
    appended(std::vector<std::size_t> ids, std::size_t new_id)
    {
      ids.push_back(new_id);
      return ids;
    }
  }

  node::node(std::vector<std::size_t> id) : id_{move(id)} {}
  node::node(std::vector<std::size_t> id, std::size_t new_id) : id_{appended(move(id), new_id)} {}

  node::~node() = default;

  std::vector<std::size_t> const&
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
