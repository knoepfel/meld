#ifndef sand_core_node_hpp
#define sand_core_node_hpp

// A node is a processing level (akin to an event).  It is a node
// because the processing model supports a tree of nodes for
// processing.  A better name is probably appropriate.

#include "sand/core/transition.hpp"

#include <iosfwd>

#include <memory>
#include <utility>
#include <vector>

namespace sand {

  class node {
  public:
    virtual ~node(); // Ick.  Should be able to type-erase this thing.
    id_t const& id() const noexcept;

  protected:
    explicit node(id_t id);
    explicit node(id_t parent_ids, std::size_t id);

  private:
    id_t id_;
  };

  using transition_messages = std::vector<std::pair<stage, std::shared_ptr<node>>>;

  // FIXME: Need better name than null_node
  class null_node_t : public node {
  public:
    null_node_t();
    ~null_node_t() final;

    // Maybe make const-qualified version of this?
    template <typename T>
    std::shared_ptr<T> make_child(std::size_t id);
  };
  extern null_node_t null_node;

  template <typename T>
  std::shared_ptr<T>
  null_node_t::make_child(std::size_t id)
  {
    return std::make_shared<T>(&null_node, id);
  }

  std::ostream& operator<<(std::ostream&, node const&);
}

#endif /* sand_core_node_hpp */
