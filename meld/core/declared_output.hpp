#ifndef meld_core_declared_output_hpp
#define meld_core_declared_output_hpp

#include "meld/concurrency.hpp"
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

    // Icky?
    incomplete_output& filtered_by(std::vector<std::string> preceding_filters)
    {
      preceding_filters_ = move(preceding_filters);
      return *this;
    }

    auto& filtered_by(std::convertible_to<std::string> auto&&... names)
    {
      return filtered_by(std::vector<std::string>{std::forward<decltype(names)>(names)...});
    }

    void concurrency(std::size_t n)
    {
      funcs_.add_output(
        name_,
        std::make_unique<declared_output>(name_, n, move(preceding_filters_), graph_, move(ft_)));
    }

  private:
    component<T>& funcs_;
    std::string name_;
    std::vector<std::string> preceding_filters_;
    tbb::flow::graph& graph_;
    detail::output_function_t ft_;
  };

  using declared_output_ptr = std::unique_ptr<declared_output>;
  using declared_outputs = std::map<std::string, declared_output_ptr>;
}

#endif /* meld_core_declared_output_hpp */
