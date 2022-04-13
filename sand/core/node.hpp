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
    virtual ~node(); // Ick.  Should be able to type-erase this thing.
    std::vector<std::size_t> const& id() const noexcept;

  protected:
    explicit node(std::vector<std::size_t> id);
    explicit node(std::vector<std::size_t> parent_ids, std::size_t id);

    template <typename T, typename Parent>
    std::shared_ptr<T>
    make_child(Parent const* parent, std::size_t id) const
    {
      return std::make_shared<T>(parent, id);
    };

  private:
    std::vector<std::size_t> id_;
  };

  class null_node_t : public node {
  public:
    null_node_t();
    ~null_node_t() final;

    template <typename T>
    std::shared_ptr<T> make_child(std::size_t id) const;
  };
  extern null_node_t const null_node;

  template <typename T>
  std::shared_ptr<T>
  null_node_t::make_child(std::size_t id) const
  {
    return node::make_child<T>(&null_node, id);
  }

  std::ostream& operator<<(std::ostream&, node const&);
}

#endif /* sand_core_node_hpp */
