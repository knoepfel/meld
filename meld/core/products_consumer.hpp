#ifndef meld_core_products_consumer_hpp
#define meld_core_products_consumer_hpp

#include "meld/core/consumer.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/message.hpp"
#include "meld/core/specified_label.hpp"
#include "meld/model/level_id.hpp"
#include "meld/model/qualified_name.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <span>
#include <string>
#include <vector>

namespace meld {
  class products_consumer : public consumer {
  public:
    products_consumer(qualified_name name, std::vector<std::string> predicates);

    virtual ~products_consumer();

    std::size_t num_inputs() const;

    tbb::flow::receiver<message>& port(specified_label const& product_label);
    virtual std::vector<tbb::flow::receiver<message>*> ports() = 0;
    virtual specified_labels input() const = 0;
    virtual std::size_t num_calls() const = 0;

  private:
    virtual tbb::flow::receiver<message>& port_for(specified_label const& product_label) = 0;
  };
}

#endif // meld_core_products_consumer_hpp
