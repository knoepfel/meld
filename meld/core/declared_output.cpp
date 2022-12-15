#include "meld/core/declared_output.hpp"

namespace meld {
  declared_output::declared_output(std::string name,
                                   std::size_t concurrency,
                                   std::vector<std::string> preceding_filters,
                                   tbb::flow::graph& g,
                                   detail::output_function_t&& ft) :
    consumer{move(name), move(preceding_filters)},
    node_{g, concurrency, [f = move(ft)](message const& msg) -> tbb::flow::continue_msg {
            if (not msg.store->is_flush()) {
              f(*msg.store);
            }
            return {};
          }}
  {
  }

  tbb::flow::receiver<message>& declared_output::port() noexcept { return node_; }
}
