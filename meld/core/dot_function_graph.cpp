#include "meld/core/dot_function_graph.hpp"
#include "meld/core/dot_attributes.hpp"

#include <iomanip>

using std::quoted;

namespace meld::dot::function_graph {
  void prolog(std::ostream& os)
  {
    os << "digraph test {\n\n"
       << "  // Framework-provided nodes and edges\n"
       << "  edge [fontname=Monaco, fontcolor=red];\n"
       << "  Source [fontname=Helvetica, shape=doublecircle, style=filled, fillcolor=gray];\n"
       << "  Multiplexer [fontname=Helvetica, shape=trapezium, style=filled, fillcolor=gray, "
          "peripheries=2];\n"
       << "  Source -> Multiplexer;\n\n"
       << "  // User-provided nodes and edges\n"
       << "  node [fontname=Monaco, peripheries=1];\n\n";
  }

  void epilog(std::ostream& os) { os << "\n}"; }

  void node_declaration(std::ostream& os,
                        std::string const& node_name,
                        std::string const& node_shape)
  {
    os << "  " << quoted(node_name) << attributes_str(shape(node_shape)) << ";\n";
  }

  void predicate_edge(std::ostream& os,
                      std::string const& source_node,
                      std::string const& target_node)
  {
    os << "  " << quoted(source_node) << " -> " << quoted(target_node)
       << attributes_str(color("red")) << ";\n";
  }

  void multiplexing_edge(std::ostream& os,
                         std::string const& source_node,
                         std::string const& target_node,
                         attributes attrs)
  {
    os << "  " << quoted(source_node) << " -> " << quoted(target_node)
       << attributes_str(
            color("blue"), style("dashed"), label(attrs.label), fontsize(default_fontsize))
       << ";\n";
  }

  void normal_edge(std::ostream& os,
                   std::string const& source_node,
                   std::string const& target_node,
                   attributes attrs)
  {
    os << "  " << quoted(source_node) << ":s -> " << quoted(target_node)
       << attributes_str(
            dir("both"), arrowtail(attrs.arrowtail), label(attrs.label), fontsize(default_fontsize))
       << ";\n";
  }

  void output_edge(std::ostream& os,
                   std::string const& source_node,
                   std::string const& target_node,
                   attributes attrs)
  {
    os << "  " << quoted(source_node) << " -> " << quoted(target_node)
       << attributes_str(dir("both"), color("gray"), arrowtail(attrs.arrowtail)) << ";\n";
  }
}
