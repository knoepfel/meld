#ifndef meld_core_edge_maker_hpp
#define meld_core_edge_maker_hpp

#include "meld/core/declared_output.hpp"
#include "meld/core/declared_splitter.hpp"
#include "meld/core/dot_attributes.hpp"
#include "meld/core/filter/result_collector.hpp"
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
    edge_maker(std::string const& filename, declared_outputs& outputs, Args&... args);

    ~edge_maker()
    {
      if (fout_) {
        dot_epilog(*fout_);
      }
    }

    template <typename... Args>
    void operator()(tbb::flow::input_node<message>& source,
                    multiplexer& multi,
                    std::map<std::string, result_collector>& filter_collectors,
                    consumers<Args>... cons);

  private:
    template <typename T>
    static std::map<product_name_t, multiplexer::named_output_port> producing_nodes(T& nodes);

    template <typename T>
    void record_attributes(T& consumers);

    template <typename T>
    multiplexer::head_ports_t edges(std::map<std::string, result_collector>& filter_collectors,
                                    T& consumers);

    static void dot_prolog(std::ostream& os);
    static void dot_epilog(std::ostream& os);

    static void dot_node_declaration(std::ostream& os,
                                     std::string const& node_name,
                                     std::string const& node_shape);
    static void dot_filter_edge(std::ostream& os,
                                std::string const& source_node,
                                std::string const& target_node);
    static void dot_multiplexing_edge(std::ostream& os,
                                      std::string const& source_node,
                                      std::string const& target_node,
                                      dot::attributes attrs);
    static void dot_normal_edge(std::ostream& os,
                                std::string const& source_node,
                                std::string const& target_node,
                                dot::attributes attrs);
    static void dot_output_edge(std::ostream& os,
                                std::string const& source_node,
                                std::string const& target_node,
                                dot::attributes attrs);

    std::unique_ptr<std::ofstream> fout_{nullptr};
    std::map<product_name_t, meld::multiplexer::named_output_port> producers_;
    declared_outputs& outputs_;
    std::map<std::string, dot::attributes> attributes_;
  };

  // =============================================================================
  // Implementation
  template <typename... Args>
  edge_maker::edge_maker(std::string const& filename, declared_outputs& outputs, Args&... args) :
    fout_{empty(filename) ? nullptr : std::make_unique<std::ofstream>(filename)}, outputs_{outputs}
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
        result[product_name] = {node_name, &node->sender(), &node->to_output()};
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
  multiplexer::head_ports_t edge_maker::edges(
    std::map<std::string, result_collector>& filter_collectors, T& consumers)
  {
    using namespace dot;
    multiplexer::head_ports_t result;
    auto const& [data, attributes] = consumers;
    for (auto& [node_name, node] : data) {
      if (fout_) {
        dot_node_declaration(*fout_, node_name, attributes.shape);
      }

      tbb::flow::receiver<message>* collector = nullptr;
      if (auto coll_it = filter_collectors.find(node_name); coll_it != cend(filter_collectors)) {
        collector = &coll_it->second.data_port();
      }

      if (fout_) {
        for (auto const& filter_name : node->filtered_by()) {
          dot_filter_edge(*fout_, filter_name, node_name);
        }
      }

      for (auto const& [product_name, domain] : node->input()) {
        auto it = producers_.find(product_name);
        auto* input_port = collector ? collector : &node->port(product_name);
        if (it == cend(producers_)) {
          // Is there a way to detect mis-specified product dependencies?
          auto const* allowed_domain = domain.empty() ? nullptr : &domain;
          result.push_back({node_name, product_name, allowed_domain, input_port});
          continue;
        }

        auto const& [sender_node, sender_name] = std::tie(it->second, it->second.node_name);
        make_edge(*sender_node.port, *input_port);
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
  void edge_maker::operator()(tbb::flow::input_node<message>& source,
                              meld::multiplexer& multi,
                              std::map<std::string, result_collector>& filter_collectors,
                              consumers<Args>... cons)
  {
    (record_attributes(cons), ...);

    make_edge(source, multi);

    // Create edges to outputs
    auto& splitters = std::get<consumers<meld::declared_splitters>&>(std::tie(cons...));

    for (auto const& [name, output_node] : outputs_) {
      make_edge(source, output_node->port());
      if (fout_) {
        dot_node_declaration(*fout_, name, "cylinder");
        dot_output_edge(*fout_, "Source", name, {});
      }
      for (auto const& [_, named_port] : producers_) {
        make_edge(*named_port.to_output, output_node->port());
        if (fout_) {
          auto const& src_attributes = attributes_.at(named_port.node_name);
          dot_output_edge(
            *fout_, named_port.node_name, name, {.arrowtail = src_attributes.arrowtail});
        }
      }
      for (auto const& [splitter_name, splitter] : splitters.data) {
        make_edge(splitter->to_output(), output_node->port());
        if (fout_) {
          auto const& src_attributes = attributes_.at(splitter_name);
          dot_output_edge(*fout_, splitter_name, name, {.arrowtail = src_attributes.arrowtail});
        }
      }
    }

    // Create normal edges
    meld::multiplexer::head_ports_t head_ports;
    auto append = [](auto& heads, auto const& new_heads) {
      heads.insert(end(heads), begin(new_heads), end(new_heads));
    };
    (append(head_ports, edges(filter_collectors, cons)), ...);

    // Create head nodes for splitters
    auto get_consumed_products = [](auto const& cons, auto& products) {
      for (auto const& [key, consumer] : cons.data) {
        for (auto const& [product_name, _] : consumer->input()) {
          products[product_name].push_back(key);
        }
      }
    };

    std::map<std::string, std::vector<std::string>> consumed_products;
    (get_consumed_products(cons, consumed_products), ...);

    std::set<std::string> remove_ports_for_products;
    for (auto const& [name, splitter] : splitters.data) {
      meld::multiplexer::head_ports_t heads;
      for (auto const& product_name : splitter->output()) {
        // There can be multiple head nodes that require the same product.
        remove_ports_for_products.insert(product_name);
        for (auto const& port : head_ports) {
          if (port.product_name != product_name) {
            continue;
          }
          heads.push_back(port);
          if (fout_) {
            dot_multiplexing_edge(*fout_, name, port.node_name, {.label = product_name});
          }
        }
      }
      splitter->finalize(std::move(heads));
    }

    // Remove head nodes claimed by splitters
    for (auto const& key : remove_ports_for_products) {
      std::erase_if(head_ports, [&key](auto const& port) { return port.product_name == key; });
    }

    if (fout_) {
      for (auto const& head_port : head_ports) {
        dot_multiplexing_edge(
          *fout_, "Multiplexer", head_port.node_name, {.label = head_port.product_name});
      }
    }

    multi.finalize(std::move(head_ports));
  }
}

#endif /* meld_core_edge_maker_hpp */
