#ifndef meld_core_multiplexer_hpp
#define meld_core_multiplexer_hpp

#include "meld/core/message.hpp"
#include "meld/model/level_id.hpp"

#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/flow_graph.h"

#include <chrono>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace meld {

  class multiplexer : public tbb::flow::function_node<message> {
    using base = tbb::flow::function_node<message>;

  public:
    ~multiplexer();

    struct named_output_port {
      std::string node_name;
      tbb::flow::sender<message>* port;
      tbb::flow::sender<message>* to_output;
    };
    struct named_input_port {
      specified_label product_label;
      tbb::flow::receiver<message>* port;
    };
    using named_input_ports_t = std::vector<named_input_port>;
    using head_ports_t = std::map<std::string, named_input_ports_t>;

    explicit multiplexer(tbb::flow::graph& g, bool debug = false);
    tbb::flow::continue_msg multiplex(message const& msg);

    void finalize(head_ports_t head_ports);

  private:
    head_ports_t head_ports_;
    bool debug_;
    std::atomic<std::size_t> received_messages_{};
    std::chrono::duration<float, std::chrono::microseconds::period> execution_time_{};
  };

}

#endif /* meld_core_multiplexer_hpp */
