#include "meld/core/edge_maker.hpp"

#include <iomanip>

namespace {
  std::string const default_fontsize{"12"};

#define DOT_ATTRIBUTE(name)                                                                        \
  inline std::string name(std::string const& str) { return #name "=" + str; }

  DOT_ATTRIBUTE(arrowtail)
  DOT_ATTRIBUTE(color)
  DOT_ATTRIBUTE(dir)
  DOT_ATTRIBUTE(fontsize)
  DOT_ATTRIBUTE(shape)
  DOT_ATTRIBUTE(style)

#undef DOT_ATTRIBUTE

  inline std::string label(std::string const& str) { return "label=\" " + str + '"'; }

  template <typename Head, typename... Tail>
  std::string attributes_str(Head const& head, Tail const&... tail)
  {
    std::string result{" [" + head};
    (result.append(", " + tail), ...);
    result.append("]");
    return result;
  }
}

using std::quoted;

namespace meld {
  void edge_maker::dot_prolog(std::ostream& os)
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

  void edge_maker::dot_epilog(std::ostream& os) { os << "\n}"; }

  void edge_maker::dot_node_declaration(std::ostream& os,
                                        std::string const& node_name,
                                        std::string const& node_shape)
  {
    os << "  " << quoted(node_name) << attributes_str(shape(node_shape)) << ";\n";
  }

  void edge_maker::dot_predicate_edge(std::ostream& os,
                                      std::string const& source_node,
                                      std::string const& target_node)
  {
    os << "  " << quoted(source_node) << " -> " << quoted(target_node)
       << attributes_str(color("red")) << ";\n";
  }

  void edge_maker::dot_multiplexing_edge(std::ostream& os,
                                         std::string const& source_node,
                                         std::string const& target_node,
                                         dot::attributes attrs)
  {
    os << "  " << quoted(source_node) << " -> " << quoted(target_node)
       << attributes_str(
            color("blue"), style("dashed"), label(attrs.label), fontsize(default_fontsize))
       << ";\n";
  }

  void edge_maker::dot_normal_edge(std::ostream& os,
                                   std::string const& source_node,
                                   std::string const& target_node,
                                   dot::attributes attrs)
  {
    os << "  " << quoted(source_node) << ":s -> " << quoted(target_node)
       << attributes_str(
            dir("both"), arrowtail(attrs.arrowtail), label(attrs.label), fontsize(default_fontsize))
       << ";\n";
  }

  void edge_maker::dot_output_edge(std::ostream& os,
                                   std::string const& source_node,
                                   std::string const& target_node,
                                   dot::attributes attrs)
  {
    os << "  " << quoted(source_node) << " -> " << quoted(target_node)
       << attributes_str(dir("both"), color("gray"), arrowtail(attrs.arrowtail)) << ";\n";
  }
}
