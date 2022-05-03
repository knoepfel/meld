#ifndef meld_core_node_hpp
#define meld_core_node_hpp

// A node is a processing level (akin to an event).  It is a node
// because the processing model supports a tree of nodes for
// processing.  A better name is probably appropriate.

#include "meld/core/transition.hpp"

#include <iosfwd>

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace meld {

  class node {
  public:
    virtual ~node(); // Ick.  Should be able to type-erase this thing.
    level_id const& id() const noexcept;
    virtual std::string_view level_name() const = 0;

  protected:
    explicit node(level_id id);
    explicit node(level_id parent_ids, std::size_t id);

  private:
    level_id id_;
  };

  using node_ptr = std::shared_ptr<node>;
  using transition_message = std::pair<transition, node_ptr>;
  using transition_messages = std::vector<transition_message>;

  transition_type ttype_for(transition_message const& msg);

  // FIXME: Need better name than null_node
  class null_node_t : public node {
  public:
    null_node_t();
    ~null_node_t() final;

    std::string_view level_name() const final;

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

#endif /* meld_core_node_hpp */
