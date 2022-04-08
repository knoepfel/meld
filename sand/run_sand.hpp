#ifndef sand_run_sand_hpp
#define sand_run_sand_hpp

#include "boost/json.hpp"

#include <cstddef>
#include <map>
#include <string>

namespace sand {
  using configurations_t = std::map<std::string, boost::json::object>;
  void run_it(configurations_t const& configurations);
}

#endif /* sand_run_sand_hpp */
