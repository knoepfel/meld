#ifndef meld_core_filter_result_collector_hpp
#define meld_core_filter_result_collector_hpp

#include "meld/core/filter/filter_impl.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/message.hpp"

#include "oneapi/tbb/flow_graph.h"

namespace meld {
  using filter_collector_base =
    oneapi::tbb::flow::composite_node<std::tuple<filter_result, message>,
                                      std::tuple<oneapi::tbb::flow::continue_msg>>;

  class result_collector : public filter_collector_base {
    using indexer_t = oneapi::tbb::flow::indexer_node<filter_result, message>;
    using tag_t = indexer_t::output_type;

  public:
    using filter_collector_base::input_ports_type;
    using filter_collector_base::output_ports_type;

    explicit result_collector(oneapi::tbb::flow::graph& g, products_consumer& consumer);
    explicit result_collector(oneapi::tbb::flow::graph& g, declared_output& output);

    auto& filter_port() { return input_port<0>(*this); }
    auto& data_port() { return input_port<1>(*this); }

  private:
    oneapi::tbb::flow::continue_msg execute(tag_t const& tag);

    decision_map decisions_;
    data_map data_;
    indexer_t indexer_;
    oneapi::tbb::flow::function_node<tag_t> filter_;
    std::vector<oneapi::tbb::flow::receiver<message>*> downstream_ports_;
    std::size_t nargs_;
  };
}

#endif /* meld_core_filter_result_collector_hpp */
