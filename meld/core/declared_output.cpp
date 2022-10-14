#include "meld/core/declared_output.hpp"

namespace meld {
  declared_output::declared_output(std::string name,
                                   std::size_t concurrency,
                                   std::vector<std::string> preceding_filters,
                                   tbb::flow::graph& g,
                                   detail::output_function_t&& ft) :
    name_{move(name)},
    preceding_filters_{move(preceding_filters)},
    node_{g, concurrency, [f = move(ft)](message const& msg) -> tbb::flow::continue_msg {
            if (not msg.store->is_flush()) {
              f(*msg.store);
            }
            return {};
          }}
  {
  }

  std::string const& declared_output::name() const noexcept { return name_; }
  std::vector<std::string> const& declared_output::filtered_by() const noexcept
  {
    return preceding_filters_;
  }
  tbb::flow::receiver<message>& declared_output::port() noexcept { return node_; }
}
