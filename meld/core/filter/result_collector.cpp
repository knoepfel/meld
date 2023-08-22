#include "meld/core/filter/result_collector.hpp"
#include "meld/core/declared_output.hpp"
#include "meld/core/products_consumer.hpp"

#include "oneapi/tbb/flow_graph.h"

using namespace meld;
using namespace oneapi::tbb;

namespace meld {
  result_collector::result_collector(flow::graph& g, products_consumer& consumer) :
    filter_collector_base{g},
    decisions_{static_cast<unsigned int>(consumer.filtered_by().size())},
    data_{consumer.input()},
    indexer_{g},
    filter_{g, flow::unlimited, [this](tag_t const& t) { return execute(t); }},
    downstream_ports_{consumer.ports()},
    nargs_{size(downstream_ports_)}
  {
    make_edge(indexer_, filter_);
    set_external_ports(input_ports_type{input_port<0>(indexer_), input_port<1>(indexer_)},
                       output_ports_type{filter_});
  }

  result_collector::result_collector(flow::graph& g, declared_output& output) :
    filter_collector_base{g},
    decisions_{static_cast<unsigned int>(output.filtered_by().size())},
    data_{data_map::for_output},
    indexer_{g},
    filter_{g, flow::unlimited, [this](tag_t const& t) { return execute(t); }},
    downstream_ports_{&output.port()},
    nargs_{size(downstream_ports_)}
  {
    make_edge(indexer_, filter_);
    set_external_ports(input_ports_type{input_port<0>(indexer_), input_port<1>(indexer_)},
                       output_ports_type{filter_});
  }

  flow::continue_msg result_collector::execute(tag_t const& t)
  {
    // FIXME: This implementation is horrible!  Because there are two data structures that
    //        have to work together.
    end_of_message_ptr eom{};
    unsigned int msg_id{};
    if (t.is_a<message>()) {
      auto const& msg = t.cast_to<message>();
      data_.update(msg.id, msg.store);
      msg_id = msg.id;
      if (msg.store->is_flush()) {
        // All flush messages are automatically forwarded to downstream ports.
        for (std::size_t i = 0ull; i != nargs_; ++i) {
          downstream_ports_[i]->try_put(msg);
        }
        return {};
      }
    }
    else {
      auto const& result = t.cast_to<filter_result>();
      decisions_.update(result);
      eom = result.eom;
      msg_id = result.msg_id;
    }

    auto const filter_decision = decisions_.value(msg_id);
    if (not is_complete(filter_decision)) {
      return {};
    }

    if (not data_.is_complete(msg_id)) {
      return {};
    }

    if (to_boolean(filter_decision)) {
      // FIXME: Can we get rid of this awful accessor?
      data_map::accessor a;
      auto const stores = data_.release_data(a, msg_id);
      if (empty(stores)) {
        return {};
      }
      for (std::size_t i = 0ull; i != nargs_; ++i) {
        downstream_ports_[i]->try_put({stores[i], eom, msg_id});
      }
    }
    decisions_.erase(msg_id);
    return {};
  }
}
