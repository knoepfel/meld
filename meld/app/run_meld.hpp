#ifndef meld_app_run_meld_hpp
#define meld_app_run_meld_hpp

#include "boost/json.hpp"

#include <optional>

namespace meld {
  void run(boost::json::value const& configurations,
           std::optional<std::string> dot_file,
           int max_parallelism);
}

#endif /* meld_app_run_meld_hpp */
