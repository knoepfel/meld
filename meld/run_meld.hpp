#ifndef meld_run_meld_hpp
#define meld_run_meld_hpp

#include "boost/json.hpp"

namespace meld {
  void run_it(boost::json::value const& configurations);
}

#endif /* meld_run_meld_hpp */
