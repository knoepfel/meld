#include "meld/core/dot_data_graph.hpp"
#include "meld/core/dot_attributes.hpp"

#include <cassert>
#include <iomanip>
#include <ostream>

namespace {
  std::string parenthesized(std::string const& n) { return "(" + n + ")"; }
}

using std::quoted;

namespace meld::dot::data_graph {
  void prolog(std::ostream& os)
  {
    os << "digraph test {\n\n"
       << "  // Framework-provided nodes and edges\n"
       << "  edge [fontname=Monaco, fontcolor=red];\n"
       << "  // User-provided nodes and edges\n"
       << "  node [fontname=Monaco, shape=plaintext];\n\n";
  }

  void epilog(std::ostream& os) { os << "\n}"; }

  void node_declaration(std::ostream& os, std::string const& node_name, attributes attrs)
  {
    os << "  " << quoted(parenthesized(node_name)) << attributes_str(fontcolor(attrs.fontcolor))
       << ";\n";
  }

  void normal_edge(std::ostream& os,
                   std::string const& source_node,
                   std::string const& target_node,
                   attributes attrs)
  {
    os << "  " << quoted(parenthesized(source_node)) << " -> " << quoted(parenthesized(target_node))
       << attributes_str(color("red"), label(attrs.label), fontsize(default_fontsize)) << ";\n";
  }

  void zip_edge(std::ostream& os, std::string const& source_node, std::string const& zip_node)
  {
    os << "  " << quoted(parenthesized(source_node)) << " -> " << quoted(parenthesized(zip_node))
       << attributes_str(color("#a9a9a9"), style("dotted"), fontsize(default_fontsize)) << ";\n";
  }

  std::string zip_node(specified_labels input)
  {
    assert(not input.empty());
    auto it = input.begin();
    auto const e = input.end();
    std::string joined = it->name;
    ++it;
    for (; it != e; ++it) {
      joined += ",";
      joined += it->name;
    }
    return joined;
  }

  std::string unzip_node(output_strings output)
  {
    assert(not output.empty());
    auto it = output.begin();
    auto const e = output.end();
    std::string joined = *it;
    ++it;
    for (; it != e; ++it) {
      joined += ",";
      joined += *it;
    }
    return joined;
  }

}
