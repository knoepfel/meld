#ifndef meld_app_load_module_hpp
#define meld_app_load_module_hpp

#include "meld/core/fwd.hpp"
#include "meld/source.hpp"

#include "boost/json.hpp"

#include <functional>

namespace meld {
  void load_module(framework_graph& g, std::string const& label, boost::json::object config);
  std::function<product_store_ptr()> load_source(boost::json::object const& config);
}

#endif /* meld_app_load_module_hpp */
