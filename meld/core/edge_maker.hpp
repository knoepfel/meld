#ifndef meld_core_edge_maker_hpp
#define meld_core_edge_maker_hpp

#include "meld/core/declared_splitter.hpp"
#include "meld/core/dot_attributes.hpp"
#include "meld/core/multiplexer.hpp"

#include <fstream>
#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

  template <typename T>
  struct consumers {
    T& data;
    dot::attributes attributes;
  };

  template <typename T>
  consumers(T&) -> consumers<T>;

  template <typename T>
  consumers(T&, dot::attributes) -> consumers<T>;

  using product_name_t = std::string;

  class edge_maker {
  public:
    template <typename... Args>
    edge_maker(std::string const& filename, Args&... args);

    ~edge_maker()
    {
      if (fout_) {
        dot_epilog(*fout_);
      }
    }

    template <typename... Args>
    void make_edges(meld::multiplexer& multi, consumers<Args>... cons);

  private:
    template <typename T>
    static std::map<product_name_t, multiplexer::named_output_port> producing_nodes(T& nodes);

    template <typename T>
    void record_attributes(T& consumers);

    template <typename T>
    multiplexer::head_nodes_t edges(T& consumers);

    static void dot_prolog(std::ostream& os);
    static void dot_epilog(std::ostream& os);

    static void dot_node_declaration(std::ostream& os,
                                     std::string const& node_name,
                                     std::string const& node_shape);
    static void dot_multiplexing_edge(std::ostream& os,
                                      std::string const& source_node,
                                      std::string const& target_node,
                                      dot::attributes attrs);
    static void dot_normal_edge(std::ostream& os,
                                std::string const& source_node,
                                std::string const& target_node,
                                dot::attributes attrs);

    std::unique_ptr<std::ofstream> fout_{nullptr};
    std::map<product_name_t, meld::multiplexer::named_output_port> producers_;
    std::map<std::string, dot::attributes> attributes_;
  };

  // =============================================================================
  // Implementation
  template <typename... Args>
  edge_maker::edge_maker(std::string const& filename, Args&... args) :
    fout_{empty(filename) ? nullptr : std::make_unique<std::ofstream>(filename)}
  {
    (producers_.merge(producing_nodes(args)), ...);
    if (fout_) {
      dot_prolog(*fout_);
    }
  }

  template <typename T>
  std::map<product_name_t, multiplexer::named_output_port> edge_maker::producing_nodes(T& nodes)
  {
    std::map<product_name_t, multiplexer::named_output_port> result;
    for (auto const& [node_name, node] : nodes) {
      for (auto const& product_name : node->output()) {
        if (empty(product_name))
          continue;
        result[product_name] = {node_name, &node->sender()};
      }
    }
    return result;
  }

  template <typename T>
  void edge_maker::record_attributes(T& consumers)
  {
    auto const& [data, attributes] = consumers;
    for (auto const& [node_name, _] : data) {
      attributes_[node_name] = attributes;
    }
  }

  template <typename T>
  multiplexer::head_nodes_t edge_maker::edges(T& consumers)
  {
    using namespace dot;
    multiplexer::head_nodes_t result;
    auto const& [data, attributes] = consumers;
    for (auto& [node_name, node] : data) {
      if (fout_) {
        dot_node_declaration(*fout_, node_name, attributes.shape);
      }
      for (auto const& product_name : node->input()) {
        auto it = producers_.find(product_name);
        if (it == cend(producers_)) {
          // Is there a way to detect mis-specified product dependencies?
          result[product_name] = {node_name, &node->port(product_name)};
          continue;
        }

        auto const& [sender_node, sender_name] = std::tie(it->second, it->second.node_name);
        make_edge(*sender_node.port, node->port(product_name));
        if (fout_) {
          auto const& src_attributes = attributes_.at(sender_name);
          dot_normal_edge(*fout_,
                          sender_name,
                          node_name,
                          {.arrowtail = src_attributes.arrowtail, .label = product_name});
        }
      }
    }
    return result;
  }

  template <typename... Args>
  void edge_maker::make_edges(meld::multiplexer& multi, consumers<Args>... cons)
  {
    (record_attributes(cons), ...);

    meld::multiplexer::head_nodes_t head_nodes;
    (head_nodes.merge(edges(cons)), ...);

    // Create head nodes for splitters
    auto get_consumed_products = [](auto const& cons, auto& products) {
      for (auto const& [key, consumer] : cons.data) {
        for (auto const& product_name : consumer->input()) {
          products[product_name].push_back(key);
        }
      }
    };

    std::map<std::string, std::vector<std::string>> consumed_products;
    (get_consumed_products(cons, consumed_products), ...);

    auto& splitters = std::get<consumers<meld::declared_splitters>&>(std::tie(cons...));

    std::set<std::string> head_nodes_to_remove;
    for (auto const& [name, splitter] : splitters.data) {
      meld::multiplexer::head_nodes_t heads;
      for (auto const& product_name : splitter->provided_products()) {
        auto it = head_nodes.find(product_name);
        heads.insert(*it);
        head_nodes_to_remove.insert(product_name);
        if (fout_) {
          dot_multiplexing_edge(*fout_, name, it->second.node_name, {.label = product_name});
        }
      }
      splitter->finalize(std::move(heads));
    }

    // Remove head nodes claimed by splitters
    for (auto const& key : head_nodes_to_remove) {
      head_nodes.erase(key);
    }

    if (fout_) {
      for (auto const& [product_name, node] : head_nodes) {
        dot_multiplexing_edge(*fout_, "Multiplexer", node.node_name, {.label = product_name});
      }
    }
    multi.finalize(move(head_nodes));
  }
}

#endif /* meld_core_edge_maker_hpp */
