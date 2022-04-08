#ifndef sand_core_load_module_hpp
#define sand_core_load_module_hpp

#include "sand/core/module_worker.hpp"
#include "sand/core/source_worker.hpp"

#include "boost/json.hpp"

#include <memory>
#include <string>

namespace sand {
  std::unique_ptr<module_worker> load_module(boost::json::value const& config);
  std::unique_ptr<source_worker> load_source(boost::json::value const& config);
}

#endif /* sand_core_load_module_hpp */
