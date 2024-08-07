#ifndef meld_core_dot_data_graph_hpp
#define meld_core_dot_data_graph_hpp

#include "meld/core/dot_attributes.hpp"
#include "meld/core/specified_label.hpp"
#include "meld/model/qualified_name.hpp"

#include <iosfwd>
#include <string>

namespace meld::dot::data_graph {
  void prolog(std::ostream& os);
  void epilog(std::ostream& os);

  void node_declaration(std::ostream& os, std::string const& node_name, attributes attrs = {});
  void normal_edge(std::ostream& os,
                   std::string const& source_node,
                   std::string const& target_node,
                   attributes attrs);
  void zip_edge(std::ostream& os, std::string const& source_node, std::string const& target_node);

  std::string zip_node(specified_labels input);
  std::string unzip_node(qualified_names output);
}
#endif /* meld_core_dot_data_graph_hpp */
