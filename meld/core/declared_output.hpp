#ifndef meld_core_declared_output_hpp
#define meld_core_declared_output_hpp

#include "meld/core/concurrency.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/message.hpp"
#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace meld {
  namespace detail {
    using output_function_t = std::function<void(message const&)>;
  }
  class declared_output {
  public:
    declared_output(std::string name,
                    std::size_t concurrency,
                    tbb::flow::graph& g,
                    detail::output_function_t&& ft) :
      name_{move(name)},
      concurrency_{concurrency},
      node_{g, concurrency_, [f = move(ft)](message const& msg) -> tbb::flow::continue_msg {
              if (not msg.store->is_flush()) {
                f(msg);
              }
              return {};
            }}
    {
    }

    std::string const& name() const noexcept;
    std::size_t concurrency() const noexcept;
    tbb::flow::receiver<message>& port() noexcept { return node_; }

  private:
    std::string name_;
    std::size_t concurrency_;
    tbb::flow::function_node<message> node_;
  };

  template <typename T>
  class incomplete_output {
  public:
    incomplete_output(component<T>& funcs,
                      std::string name,
                      tbb::flow::graph& g,
                      detail::output_function_t&& f) :
      funcs_{funcs}, name_{move(name)}, graph_{g}, ft_{move(f)}
    {
    }

    void concurrency(std::size_t n)
    {
      funcs_.add_output(name_, std::make_unique<declared_output>(name_, n, graph_, move(ft_)));
    }

  private:
    component<T>& funcs_;
    std::string name_;
    tbb::flow::graph& graph_;
    detail::output_function_t ft_;
  };

  using declared_output_ptr = std::unique_ptr<declared_output>;
  using declared_outputs = std::map<std::string, declared_output_ptr>;
}

#endif /* meld_core_declared_output_hpp */
