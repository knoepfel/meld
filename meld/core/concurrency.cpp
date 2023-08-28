#include "meld/concurrency.hpp"

#include "oneapi/tbb/flow_graph.h"

namespace meld {
  concurrency const concurrency::unlimited{tbb::flow::unlimited};
  concurrency const concurrency::serial{tbb::flow::serial};
}
