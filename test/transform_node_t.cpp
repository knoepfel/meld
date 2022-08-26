#include "meld/core/product_store.hpp"
#include "meld/utilities/debug.hpp"
#include "meld/utilities/sized_tuple.hpp"

#include "catch2/catch.hpp"
#include "oneapi/tbb/flow_graph.h"

#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

using namespace meld;
using namespace oneapi::tbb;

namespace {
  template <std::size_t N>
  using stores_t = sized_tuple<product_store_ptr, N>;

  template <std::size_t N>
  using join_product_stores_t = flow::join_node<stores_t<N>, tbb::flow::tag_matching>;

  struct no_join {
    no_join(flow::graph& g, ProductStoreHasher) :
      pass_through{
        g, flow::unlimited, [](product_store_ptr const& store) { return std::tuple{store}; }}
    {
    }
    flow::function_node<product_store_ptr, stores_t<1ull>> pass_through;
  };

  template <std::size_t N>
  using join_or_none_t = std::conditional_t<N == 1ull, no_join, join_product_stores_t<N>>;

  class base_node {
  public:
    virtual ~base_node() = default;
    flow::receiver<product_store_ptr>&
    port(std::string const& product_name)
    {
      return port_for(product_name);
    }
    virtual flow::sender<product_store_ptr>& sender() = 0;
    virtual std::span<std::string const, std::dynamic_extent> input() const = 0;
    virtual std::span<std::string const, std::dynamic_extent> output() const = 0;

  private:
    virtual flow::receiver<product_store_ptr>& port_for(std::string const& product_name) = 0;
  };

  template <std::size_t N>
  class transform_node : public base_node {
    template <typename R, typename... Args, std::size_t... Is>
    R
    call(R (*f)(Args...), stores_t<N> const& stores, std::index_sequence<Is...>)
    {
      return (*f)(std::get<Is>(stores)->template get_product<Args>(input_[Is])...);
    }

    std::size_t
    port_index_for(std::string const& product_name)
    {
      auto it = std::find(cbegin(input_), cend(input_), product_name);
      if (it == cend(input_)) {
        throw std::runtime_error("Product name " + product_name + " not valid for transform.");
      }
      return std::distance(cbegin(input_), it);
    }

    template <std::size_t I>

    flow::receiver<product_store_ptr>&
    receiver_for(std::size_t const index)
    {
      if constexpr (I < N) {
        if (I != index) {
          return receiver_for<I + 1ull>(index);
        }
        return input_port<I>(join_);
      }
      else {
        throw std::runtime_error("Should never get here");
      }
    }

  public:
    template <typename R, typename... Args>
    explicit transform_node(flow::graph& g,
                            R (*f)(Args...),
                            std::array<std::string, N> const& input,
                            std::string output = {}) :
      input_{input},
      output_{move(output)},
      join_{g, type_for_t<ProductStoreHasher, Args>{}...},
      transform_{g, flow::unlimited, [this, f](stores_t<N> const& stores) {
                   auto store = make_product_store();
                   if constexpr (std::same_as<R, void>) {
                     call(f, stores, std::index_sequence_for<Args...>{});
                   }
                   else {
                     auto result = call(f, stores, std::index_sequence_for<Args...>{});
                     store->add_product(output_[0], result);
                   }
                   return store;
                 }}
    {
      if constexpr (N > 1ull) {
        make_edge(join_, transform_);
      }
      else {
        make_edge(join_.pass_through, transform_);
      }
    }

  private:
    flow::receiver<product_store_ptr>&
    port_for(std::string const& product_name) override
    {
      if constexpr (N > 1ull) {
        auto const index = port_index_for(product_name);
        return receiver_for<0ull>(index);
      }
      else {
        return join_.pass_through;
      }
    }

    flow::sender<product_store_ptr>&
    sender() override
    {
      return transform_;
    }

    std::span<std::string const, std::dynamic_extent>
    input() const override
    {
      return input_;
    }
    std::span<std::string const, std::dynamic_extent>
    output() const override
    {
      return output_;
    }

    std::array<std::string, N> input_;
    std::array<std::string, 1ull> output_;
    join_or_none_t<N> join_;
    flow::function_node<stores_t<N>, product_store_ptr> transform_;
  };

  void
  make_edge(base_node& node, flow::receiver<product_store_ptr>& rec)
  {
    make_edge(node.sender(), rec);
  }

  //  void foo(int) {}
  double
  bar(int i, double d)
  {
    debug("Received ", i, " and ", d);
    return d + i;
  }
  void
  baz(double d)
  {
    debug("Received ", d);
  }

  class fgraph {
  public:
    template <typename R, typename... Args, std::size_t N = sizeof...(Args)>
    void
    add_transform(std::string name,
                  R (*f)(Args...),
                  std::array<std::string, N> const& input,
                  std::string output = {})
    {
      debug("Adding transform node with size of ", sizeof(transform_node<N>), " bytes");
      nodes.try_emplace(move(name),
                        std::make_unique<transform_node<N>>(graph, f, input, move(output)));
    }

    void
    finalize()
    {
      using product_name_t = std::string;
      using node_name_t = std::string;
      std::map<product_name_t, node_name_t> produced_products;
      for (auto const& [node_name, node] : nodes) {
        for (auto const& product_name : node->output()) {
          if (empty(product_name))
            continue;
          produced_products[product_name] = node_name;
        }
      }
      for (auto& [node_name, node] : nodes) {
        for (auto const& product_name : node->input()) {
          auto it = produced_products.find(product_name);
          if (it != cend(produced_products)) {
            make_edge(*nodes.at(it->second), node->port(product_name));
          }
          head_nodes[product_name] = &node->port(product_name);
        }
      }
    }

    void
    process(product_store_ptr const& store)
    {
      for (auto const& [name, _] : *store) {
        if (auto it = head_nodes.find(name); it != cend(head_nodes)) {
          it->second->try_put(store);
        }
      }
      graph.wait_for_all();
    }

  private:
    flow::graph graph;
    std::map<std::string, flow::receiver<product_store_ptr>*> head_nodes;
    std::map<std::string, std::unique_ptr<base_node>> nodes;
  };
}

TEST_CASE("Call non-framework functions", "[programming model]")
{
  fgraph fg;
  fg.add_transform("bar", bar, std::array<std::string, 2ull>{"test", "me"}, "bigger");
  fg.add_transform("baz", baz, std::array<std::string, 1ull>{"bigger"});

  auto store = make_product_store();
  store->add_product("test", 1);
  store->add_product("me", 2.5);
  fg.finalize();
  fg.process(store);
}
