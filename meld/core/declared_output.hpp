#ifndef meld_core_declared_output_hpp
#define meld_core_declared_output_hpp

#include "meld/concurrency.hpp"
#include "meld/core/common_node_options.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/message.hpp"
#include "meld/core/product_store.hpp"
#include "meld/core/registrar.hpp"
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
    using output_function_t = std::function<void(product_store const&)>;
  }
  class declared_output {
  public:
    declared_output(std::string name,
                    std::size_t concurrency,
                    std::vector<std::string> preceding_filters,
                    tbb::flow::graph& g,
                    detail::output_function_t&& ft);

    std::string const& name() const noexcept;
    std::vector<std::string> const& filtered_by() const noexcept;
    tbb::flow::receiver<message>& port() noexcept;

  private:
    std::string name_;
    std::vector<std::string> preceding_filters_;
    tbb::flow::function_node<message> node_;
  };

  using declared_output_ptr = std::unique_ptr<declared_output>;
  using declared_outputs = std::map<std::string, declared_output_ptr>;

  class output_creator : public common_node_options<output_creator> {
    using common_node_options_t = common_node_options<output_creator>;

  public:
    output_creator(registrar<declared_outputs> reg,
                   std::string name,
                   tbb::flow::graph& g,
                   detail::output_function_t&& f) :
      common_node_options_t{this}, name_{move(name)}, graph_{g}, ft_{move(f)}, reg_{std::move(reg)}
    {
      reg_.set([this] { return create(); });
    }

    declared_output_ptr create()
    {
      return std::make_unique<declared_output>(move(name_),
                                               common_node_options_t::concurrency(),
                                               common_node_options_t::release_preceding_filters(),
                                               graph_,
                                               move(ft_));
    }

  private:
    std::string name_;
    tbb::flow::graph& graph_;
    detail::output_function_t ft_;
    registrar<declared_outputs> reg_;
  };
}

#endif /* meld_core_declared_output_hpp */
