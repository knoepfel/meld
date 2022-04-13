#include "sand/core/node.hpp"

#include <ostream>

namespace sand {
  node::node(std::size_t id) : id_{id} {}
  node::~node() = default;

  std::vector<std::size_t> const&
  node::id() const noexcept
  {
    return id_;
  }

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
