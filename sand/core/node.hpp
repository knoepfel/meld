#ifndef sand_core_node_hpp
#define sand_core_node_hpp

// A node is a processing level (akin to an event).  It is a node
// because the processing model supports a tree of nodes for
// processing.  A better name is probably appropriate.

#include <cstddef>
#include <iosfwd>
#include <vector>

namespace sand {
  class node {
  public:
    explicit node(std::size_t id);
    virtual ~node(); // Ick.  Should be able to type-erase this thing.
    std::vector<std::size_t> const& id() const noexcept;

  private:
    std::vector<std::size_t> id_;
  };

  std::ostream& operator<<(std::ostream&, node const&);
}

#endif /* sand_core_node_hpp */
