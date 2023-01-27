#ifndef meld_model_transition_hpp
#define meld_model_transition_hpp

#include "meld/model/fwd.hpp"
#include "meld/model/level_id.hpp"

#include <iosfwd>
#include <string>
#include <vector>

namespace meld {
  std::string to_string(stage);
  std::ostream& operator<<(std::ostream& os, transition const& t);
}

#endif /* meld_model_transition_hpp */
