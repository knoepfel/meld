#include "meld/core/declared_output.hpp"

namespace meld {
  declared_output::declared_output(std::string name,
                                   std::size_t concurrency,
                                   std::vector<std::string> predicates,
                                   tbb::flow::graph& g,
                                   detail::output_function_t&& ft) :
    consumer{std::move(name), std::move(predicates)},
    node_{g, concurrency, [f = std::move(ft)](message const& msg) -> tbb::flow::continue_msg {
            if (not msg.store->is_flush()) {
              f(*msg.store);
            }
            return {};
          }}
  {
  }

  tbb::flow::receiver<message>& declared_output::port() noexcept { return node_; }
}
