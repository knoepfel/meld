#ifndef meld_core_dot_attributes_hpp
#define meld_core_dot_attributes_hpp

#include <string>

namespace meld::dot {
  struct attributes {
    std::string arrowtail{"none"};
    std::string label;
    std::string shape;
  };
}

#endif /* meld_core_dot_attributes_hpp */
