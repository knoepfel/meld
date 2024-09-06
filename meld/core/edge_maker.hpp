#ifndef meld_core_edge_maker_hpp
#define meld_core_edge_maker_hpp

#include "meld/core/declared_output.hpp"
#include "meld/core/declared_splitter.hpp"
#include "meld/core/dot/attributes.hpp"
#include "meld/core/dot/data_graph.hpp"
#include "meld/core/dot/function_graph.hpp"
#include "meld/core/filter.hpp"
#include "meld/core/multiplexer.hpp"

#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

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
    edge_maker(std::string const& file_prefix, Args&... args);

    template <typename... Args>
    void operator()(tbb::flow::input_node<message>& source,
                    multiplexer& multi,
                    std::map<std::string, filter>& filters,
                    declared_outputs& outputs,
                    consumers<Args>... cons);

    auto release_data_graph() { return std::move(data_graph_); }
    auto release_function_graph() { return std::move(function_graph_); }

  private:
    struct named_output_port {
      std::string node_name;
      tbb::flow::sender<message>* port;
      tbb::flow::sender<message>* to_output;
    };

    template <typename T>
    static std::map<product_name_t, named_output_port> producing_nodes(T& nodes);

    template <typename T>
    void record_attributes(T& consumers);

    template <typename T>
    multiplexer::head_ports_t edges(std::map<std::string, filter>& filters, T& consumers);

    std::unique_ptr<dot::function_graph> function_graph_;
    std::unique_ptr<dot::data_graph> data_graph_;

    std::map<product_name_t, named_output_port> producers_;
    std::map<std::string, dot::attributes> attributes_;

    template <typename T>
    void make_the_node(T& node, dot::attributes const& node_attributes)
    {
      if (not function_graph_) {
        return;
      }

      auto const& node_name = node->full_name();
      function_graph_->node(node_name, node_attributes);
      for (auto const& predicate_name : node->when()) {
        function_graph_->edge(predicate_name, node_name, {.color = "red"});
      }

      if constexpr (supports_output<decltype(node)>) {
        if (data_graph_) {
          data_graph_->add(node_name, node->input(), node->output());
        }
      }
    }

    template <typename Sender, typename Receiver>
    void make_the_edge(Sender& sender,
                       Receiver& receiver,
                       std::string const& receiver_node_name,
                       std::string const& product_name)
    {
      make_edge(*sender.port, receiver);
      if (function_graph_) {
        function_graph_->edge(sender.node_name,
                              receiver_node_name,
                              {.color = "blue",
                               .fontsize = dot::default_fontsize,
                               .label = dot::parenthesized(product_name)});
      }
    }
  };

  // =============================================================================
  // Implementation
  template <typename... Args>
  edge_maker::edge_maker(std::string const& file_prefix, Args&... producers) :
    function_graph_{file_prefix.empty() ? nullptr : std::make_unique<dot::function_graph>()},
    data_graph_{file_prefix.empty() ? nullptr : std::make_unique<dot::data_graph>()}
  {
    (producers_.merge(producing_nodes(producers)), ...);
  }

  template <typename T>
  std::map<product_name_t, edge_maker::named_output_port> edge_maker::producing_nodes(T& nodes)
  {
    std::map<product_name_t, named_output_port> result;
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
    for (auto const& node_name : data | std::views::keys) {
      attributes_[node_name] = attributes;
    }
  }

  template <typename T>
  multiplexer::head_ports_t edge_maker::edges(std::map<std::string, filter>& filters, T& consumers)
  {
    multiplexer::head_ports_t result;
    auto const& [data, attributes] = consumers;
    for (auto& [node_name, node] : data) {
      tbb::flow::receiver<message>* collector = nullptr;
      if (auto coll_it = filters.find(node_name); coll_it != cend(filters)) {
        collector = &coll_it->second.data_port();
      }

      make_the_node(node, attributes);

      for (auto const& product_label : node->input()) {
        auto* receiver_port = collector ? collector : &node->port(product_label);
        auto it = producers_.find(product_label.name.full("/"));
        if (it == cend(producers_)) {
          // Is there a way to detect mis-specified product dependencies?
          result[node_name].push_back({product_label, receiver_port});
          continue;
        }

        make_the_edge(it->second, *receiver_port, node_name, to_name(product_label));
      }
    }
    return result;
  }

  template <typename... Args>
  void edge_maker::operator()(tbb::flow::input_node<message>& source,
                              multiplexer& multi,
                              std::map<std::string, filter>& filters,
                              declared_outputs& outputs,
                              consumers<Args>... cons)
  {
    (record_attributes(cons), ...);

    make_edge(source, multi);

    // Create edges to outputs
    auto& splitters = std::get<consumers<declared_splitters>&>(std::tie(cons...));

    for (auto const& [output_name, output_node] : outputs) {
      make_edge(source, output_node->port());
      if (function_graph_) {
        function_graph_->node(output_name, {.shape = "cylinder"});
        function_graph_->edge("Source", output_name, {.color = "gray"});
      }
      for (auto const& named_port : producers_ | std::views::values) {
        make_edge(*named_port.to_output, output_node->port());
        if (function_graph_) {
          function_graph_->edge(named_port.node_name, output_name, {.color = "gray"});
        }
      }
      for (auto const& [splitter_name, splitter] : splitters.data) {
        make_edge(splitter->to_output(), output_node->port());
        if (function_graph_) {
          function_graph_->edge(splitter_name, output_name, {.color = "gray"});
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
          }
        }
      }
      splitter->finalize(std::move(heads));
    }

    // Remove head nodes claimed by splitters
    for (auto const& key : remove_ports_for_products) {
      for (auto& ports : head_ports | std::views::values) {
        std::erase_if(ports,
                      [&key](auto const& port) { return to_name(port.product_label) == key; });
      }
    }

    multi.finalize(std::move(head_ports));

    if (function_graph_) {
      for (auto const& [name, splitter] : splitters.data) {
        for (auto const& [node_name, ports] : splitter->downstream_ports()) {
          function_graph_->edges_for(name, node_name, ports);
        }
      }

      for (auto const& [node_name, ports] : multi.downstream_ports()) {
        function_graph_->edges_for("Source", node_name, ports);
      }
    }
  }
}

#endif // meld_core_edge_maker_hpp
