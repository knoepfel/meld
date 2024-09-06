#ifndef meld_core_dot_function_graph_hpp
#define meld_core_dot_function_graph_hpp

#include "meld/core/dot/attributes.hpp"
#include "meld/core/multiplexer.hpp"

#include <iosfwd>
#include <string>
#include <vector>

namespace meld::dot {
  class function_graph {
  public:
    void node(std::string const& node_name, attributes const& attrs);
    void edge(std::string const& source_node,
              std::string const& target_node,
              attributes const& attrs);
    void edges_for(std::string const& source_name,
                   std::string const& target_name,
                   multiplexer::named_input_ports_t const& target_ports);

    void to_file(std::string const& file_prefix) const;

  private:
    struct function_node {
      std::string name;
      attributes attrs;
    };
    std::vector<function_node> nodes_;

    struct function_edge {
      std::string source;
      std::string target;
      attributes attrs;
    };
    std::vector<function_edge> edges_;
  };
}
#endif // meld_core_dot_function_graph_hpp
