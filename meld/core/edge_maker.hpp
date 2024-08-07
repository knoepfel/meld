#ifndef meld_core_edge_maker_hpp
#define meld_core_edge_maker_hpp

#include "meld/core/declared_output.hpp"
#include "meld/core/declared_splitter.hpp"
#include "meld/core/dot_attributes.hpp"
#include "meld/core/dot_data_graph.hpp"
#include "meld/core/dot_function_graph.hpp"
#include "meld/core/filter.hpp"
#include "meld/core/multiplexer.hpp"

#include <fstream>
#include <iosfwd>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

  inline std::string const zip_edge{"[zip]"};

  template <typename T>
  concept supports_output = requires(T t) {
    { t->output() };
  };

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
    edge_maker(std::string const& file_prefix, declared_outputs& outputs, Args&... args);

    template <typename... Args>
    void operator()(tbb::flow::input_node<message>& source,
                    multiplexer& multi,
                    std::map<std::string, filter>& filters,
                    consumers<Args>... cons);

  private:
    template <typename T>
    static std::map<product_name_t, multiplexer::named_output_port> producing_nodes(T& nodes);

    template <typename T>
    void record_attributes(T& consumers);

    template <typename T>
    multiplexer::head_ports_t edges(std::map<std::string, filter>& filters, T& consumers);

    struct dot_files {
      std::ofstream function_graph;
      std::ofstream pre_data_graph;
      std::ofstream post_data_graph;
    };
    static std::unique_ptr<dot_files> maybe_graph_files(std::string const& filename);

    std::unique_ptr<dot_files> graph_files_;
    std::map<product_name_t, multiplexer::named_output_port> producers_;

    struct target_with_edge {
      std::string target;
      std::string edge;
    };
    std::map<product_name_t, target_with_edge> data_graph_edges_;
    std::map<product_name_t, dot::attributes> data_graph_nodes_;
    declared_outputs& outputs_;
    std::map<std::string, dot::attributes> attributes_;
  };

  // =============================================================================
  // Implementation
  template <typename... Args>
  edge_maker::edge_maker(std::string const& file_prefix, declared_outputs& outputs, Args&... args) :
    graph_files_{maybe_graph_files(file_prefix)}, outputs_{outputs}
  {
    (producers_.merge(producing_nodes(args)), ...);
  }

  template <typename T>
  std::map<product_name_t, multiplexer::named_output_port> edge_maker::producing_nodes(T& nodes)
  {
    std::map<product_name_t, multiplexer::named_output_port> result;
    for (auto const& [node_name, node] : nodes) {
      for (auto const& product_name : node->output()) {
        if (empty(product_name.name()))
          continue;
        result[product_name.full("/")] = {node_name, &node->sender(), &node->to_output()};
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
  multiplexer::head_ports_t edge_maker::edges(std::map<std::string, filter>& filters, T& consumers)
  {
    using namespace dot;
    multiplexer::head_ports_t result;
    auto const& [data, attributes] = consumers;
    for (auto& [node_name, node] : data) {
      if (graph_files_) {
        dot::function_graph::node_declaration(
          graph_files_->function_graph, node_name, attributes.shape);
      }

      tbb::flow::receiver<message>* collector = nullptr;
      if (auto coll_it = filters.find(node_name); coll_it != cend(filters)) {
        collector = &coll_it->second.data_port();
      }

      if (graph_files_) {
        for (auto const& predicate_name : node->when()) {
          dot::function_graph::predicate_edge(
            graph_files_->function_graph, predicate_name, node_name);
        }
      }

      for (auto const& product_label : node->input()) {
        auto* input_port = collector ? collector : &node->port(product_label);
        auto it = producers_.find(product_label.name.full("/"));
        if (it == cend(producers_)) {
          // Is there a way to detect mis-specified product dependencies?
          result[node_name].push_back({product_label, input_port});
          continue;
        }

        auto const& [sender_node, sender_name] = std::tie(it->second, it->second.node_name);
        make_edge(*sender_node.port, *input_port);
        if (graph_files_) {
          auto const& src_attributes = attributes_.at(sender_name);
          dot::function_graph::normal_edge(
            graph_files_->function_graph,
            sender_name,
            node_name,
            {.arrowtail = src_attributes.arrowtail, .label = to_name(product_label)});
        }
      }

      if constexpr (supports_output<decltype(node)>) {
        if (graph_files_) {
          // Data-graph nodes
          std::string source_name{};
          if (node->input().size() > 1ull) {
            auto zip_node_name = dot::data_graph::zip_node(node->input());
            for (auto const& product_name : node->input()) {
              dot::data_graph::zip_edge(
                graph_files_->pre_data_graph, product_name.name.full(), zip_node_name);
              data_graph_edges_[product_name.name.full()] = {zip_node_name, zip_edge};
              data_graph_nodes_[product_name.name.full()];
            }
            source_name = std::move(zip_node_name);
            dot::data_graph::node_declaration(graph_files_->pre_data_graph,
                                              source_name,
                                              {.label = source_name, .fontcolor = "gray"});
          }
          else {
            auto const& name = node->input().begin()->name;
            source_name = name.full();
            dot::data_graph::node_declaration(
              graph_files_->pre_data_graph, source_name, {.label = name.name()});
          }
          data_graph_nodes_[source_name];

          std::string target_name{};
          if (node->output().size() > 1ull) {
            auto unzip_node_name = dot::data_graph::unzip_node(node->output());
            for (auto const& product_name : node->output()) {
              dot::data_graph::node_declaration(
                graph_files_->pre_data_graph, product_name.full(), {.label = product_name.name()});
              dot::data_graph::zip_edge(
                graph_files_->pre_data_graph, unzip_node_name, product_name.full());
              data_graph_edges_[unzip_node_name] = {product_name.full(), zip_edge};
              data_graph_nodes_[product_name.full()];
            }
            target_name = std::move(unzip_node_name);
            dot::data_graph::node_declaration(graph_files_->pre_data_graph,
                                              target_name,
                                              {.label = target_name, .fontcolor = "\"#a9a9a9\""});
          }
          else {
            auto const& name = *node->output().begin();
            target_name = name.full();
            dot::data_graph::node_declaration(
              graph_files_->pre_data_graph, target_name, {.label = name.name()});
          }
          data_graph_nodes_[target_name];

          dot::data_graph::normal_edge(
            graph_files_->pre_data_graph, source_name, target_name, {.label = node->full_name()});
          data_graph_edges_[source_name] = {target_name, node->full_name()};
        }
      }
    }
    return result;
  }

  template <typename... Args>
  void edge_maker::operator()(tbb::flow::input_node<message>& source,
                              multiplexer& multi,
                              std::map<std::string, filter>& filters,
                              consumers<Args>... cons)
  {
    if (graph_files_) {
      dot::function_graph::prolog(graph_files_->function_graph);
      dot::data_graph::prolog(graph_files_->pre_data_graph);
    }

    (record_attributes(cons), ...);

    make_edge(source, multi);

    // Create edges to outputs
    auto& splitters = std::get<consumers<declared_splitters>&>(std::tie(cons...));

    for (auto const& [name, output_node] : outputs_) {
      make_edge(source, output_node->port());
      if (graph_files_) {
        dot::function_graph::node_declaration(graph_files_->function_graph, name, "cylinder");
        dot::function_graph::output_edge(graph_files_->function_graph, "Source", name, {});
      }
      for (auto const& [_, named_port] : producers_) {
        make_edge(*named_port.to_output, output_node->port());
        if (graph_files_) {
          auto const& src_attributes = attributes_.at(named_port.node_name);
          dot::function_graph::output_edge(graph_files_->function_graph,
                                           named_port.node_name,
                                           name,
                                           {.arrowtail = src_attributes.arrowtail});
        }
      }
      for (auto const& [splitter_name, splitter] : splitters.data) {
        make_edge(splitter->to_output(), output_node->port());
        if (graph_files_) {
          auto const& src_attributes = attributes_.at(splitter_name);
          dot::function_graph::output_edge(graph_files_->function_graph,
                                           splitter_name,
                                           name,
                                           {.arrowtail = src_attributes.arrowtail});
        }
      }
    }

    // Create normal edges
    multiplexer::head_ports_t head_ports;
    (head_ports.merge(edges(filters, cons)), ...);

    // Create head nodes for splitters
    auto get_consumed_products = [](auto const& cons, auto& products) {
      for (auto const& [key, consumer] : cons.data) {
        for (auto const& product_name : consumer->input() | std::views::transform(to_name)) {
          products[product_name].push_back(key);
        }
      }
    };

    std::map<std::string, std::vector<std::string>> consumed_products;
    (get_consumed_products(cons, consumed_products), ...);

    std::set<std::string> remove_ports_for_products;
    for (auto const& [name, splitter] : splitters.data) {
      multiplexer::head_ports_t heads;
      for (auto const& product_name : splitter->output()) {
        // There can be multiple head nodes that require the same product.
        remove_ports_for_products.insert(product_name.full());
        for (auto const& [node_name, ports] : head_ports) {
          for (auto const& port : ports) {
            if (to_name(port.product_label) != product_name.name()) {
              continue;
            }
            heads[node_name].push_back(port);
            if (graph_files_) {
              dot::function_graph::multiplexing_edge(
                graph_files_->function_graph, name, node_name, {.label = product_name.name()});
            }
          }
        }
      }
      splitter->finalize(std::move(heads));
    }

    // Remove head nodes claimed by splitters
    for (auto const& key : remove_ports_for_products) {
      for (auto& [_, ports] : head_ports) {
        std::erase_if(ports,
                      [&key](auto const& port) { return to_name(port.product_label) == key; });
      }
    }

    if (graph_files_) {
      for (auto const& [node_name, ports] : head_ports) {
        for (auto const& head_port : ports) {
          dot::function_graph::multiplexing_edge(graph_files_->function_graph,
                                                 "Multiplexer",
                                                 node_name,
                                                 {.label = head_port.product_label.to_string()});
        }
      }
    }

    multi.finalize(std::move(head_ports));

    if (graph_files_) {
      dot::function_graph::epilog(graph_files_->function_graph);
      dot::data_graph::epilog(graph_files_->pre_data_graph);
    }
  }
}

#endif /* meld_core_edge_maker_hpp */
