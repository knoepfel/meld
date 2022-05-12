#ifndef meld_core_load_module_hpp
#define meld_core_load_module_hpp

#include "meld/core/source_worker.hpp"
#include "meld/graph/module_worker.hpp"

#include "boost/json.hpp"

#include <memory>
#include <string>

namespace meld {
  std::unique_ptr<module_worker> load_module(boost::json::value const& config);
  std::unique_ptr<source_worker> load_source(boost::json::value const& config);
}

#endif /* meld_core_load_module_hpp */
