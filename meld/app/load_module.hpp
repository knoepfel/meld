#ifndef meld_app_load_module_hpp
#define meld_app_load_module_hpp

#include "meld/core/fwd.hpp"
#include "meld/source.hpp"

#include "boost/json.hpp"

#include <functional>

namespace meld {
  void load_module(framework_graph& g, std::string const& label, boost::json::object config);
  detail::next_store_t load_source(boost::json::object const& config);
}

#endif // meld_app_load_module_hpp
