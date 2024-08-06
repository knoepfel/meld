#ifndef meld_core_dot_function_graph_hpp
#define meld_core_dot_function_graph_hpp

#include "meld/core/dot_attributes.hpp"

#include <iosfwd>
#include <string>

namespace meld::dot::function_graph {
  void prolog(std::ostream& os);
  void epilog(std::ostream& os);

  void node_declaration(std::ostream& os,
                        std::string const& node_name,
                        std::string const& node_shape);
  void predicate_edge(std::ostream& os,
                      std::string const& source_node,
                      std::string const& target_node);
  void multiplexing_edge(std::ostream& os,
                         std::string const& source_node,
                         std::string const& target_node,
                         attributes attrs);
  void normal_edge(std::ostream& os,
                   std::string const& source_node,
                   std::string const& target_node,
                   attributes attrs);
  void output_edge(std::ostream& os,
                   std::string const& source_node,
                   std::string const& target_node,
                   attributes attrs);
}
#endif /* meld_core_dot_function_graph_hpp */
