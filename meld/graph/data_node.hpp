#ifndef meld_graph_data_node_hpp
#define meld_graph_data_node_hpp

// A data_node is a processing level (akin to an event).  It is a node
// because the processing model supports a tree of nodes for processing.

#include "meld/graph/transition.hpp"

#include <iosfwd>

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace meld {

  class data_node {
  public:
    level_id const& id() const noexcept;
    std::string_view level_name() const noexcept;

  protected:
    explicit data_node(level_id id, std::string_view name);
    explicit data_node(level_id const& parent_id, std::size_t id, std::string_view name);

  private:
    level_id id_;
    std::string_view name_;
  };

  using data_node_ptr = std::shared_ptr<data_node>;
  using transition_message = std::pair<transition, data_node_ptr>;
  using transition_messages = std::vector<transition_message>;

  transition_type ttype_for(transition_message const& msg);

  class root_node_t : public data_node {
  public:
    root_node_t();

    // Maybe make const-qualified version of this?
    template <typename T>
    std::shared_ptr<T> make_child(std::size_t id);
  };
  extern std::shared_ptr<root_node_t> root_node;

  template <typename T>
  std::shared_ptr<T>
  root_node_t::make_child(std::size_t id)
  {
    return std::make_shared<T>(root_node.get(), id);
  }

  std::ostream& operator<<(std::ostream&, data_node const&);
}

#endif /* meld_graph_data_node_hpp */
