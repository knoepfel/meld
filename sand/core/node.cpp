#include "sand/core/node.hpp"

#include <ostream>

namespace sand {
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

  null_node_t::null_node_t() : node{{}} {}
  null_node_t::~null_node_t() = default;
  null_node_t null_node{};

  std::ostream&
  operator<<(std::ostream& os, node const& n)
  {
    auto const& id = n.id();
    os << '[' << id.front();
    for (auto b = begin(id) + 1, e = end(id); b != e; ++b) {
      os << ", " << *b;
    }
    os << ']';
    return os;
  }

}
