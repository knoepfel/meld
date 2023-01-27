#include "meld/model/level_id.hpp"

#include <ostream>

namespace meld {

  std::string to_string(stage const s)
  {
    switch (s) {
    case stage::process:
      return "process";
    case stage::flush:
      return "flush";
    }
    return {};
  }

  std::ostream& operator<<(std::ostream& os, transition const& t)
  {
    return os << "ID: " << t.first << " Stage: " << to_string(t.second);
  }
}
